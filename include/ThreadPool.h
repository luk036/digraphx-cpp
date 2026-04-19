/*!
 * @file ThreadPool.h
 * @brief Thread pool implementation for parallel task execution
 *
 * This module provides a thread pool implementation that enables parallel
 * execution of tasks using a fixed number of worker threads. Tasks are
 * submitted to a queue and executed by available workers.
 *
 * Key features:
 * - Fixed number of worker threads
 * - Thread-safe task queue
 * - std::future-based result retrieval
 * - Automatic thread cleanup on destruction
 *
 * Example usage:
 * ```cpp
 * ThreadPool pool(4);  // Create pool with 4 workers
 *
 * // Submit a task and get a future
 * auto result = pool.enqueue([](int x) { return x * 2; }, 21);
 *
 * // Wait for result
 * std::cout << result.get() << std::endl;  // Outputs: 42
 * ```
 *
 * Performance characteristics:
 * - Task submission: O(1) amortized
 * - Task execution: Dependent on worker availability
 * - Memory: O(threads) for worker threads + O(queued tasks)
 */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

/*!
 * @brief Thread pool for parallel task execution
 *
 * This class implements a thread pool that manages a fixed number of worker
 * threads. Tasks are submitted to a thread-safe queue and executed by the
 * next available worker.
 *
 * The pool uses a producer-consumer pattern:
 * - Tasks are enqueued by main threads
 * - Worker threads consume and execute tasks from the queue
 * - Results are returned via std::future objects
 *
 * Thread safety:
 * - Task queue is protected by mutex and condition variable
 * - Multiple threads can safely enqueue tasks
 * - Worker threads terminate gracefully on destruction
 *
 * @note The pool does not support cancellation of queued tasks
 * @note All threads are joined in the destructor - this may block
 */
class ThreadPool {
  public:
    /*!
     * @brief Construct a thread pool with the specified number of workers
     *
     * Creates a new thread pool and spawns the specified number of worker
     * threads. Each worker runs continuously, waiting for tasks to execute.
     *
     * @param threads Number of worker threads to create
     * @throw std::runtime_error if thread creation fails
     *
     * Example:
     * ```cpp
     * ThreadPool pool(4);  // Create 4 worker threads
     * ```
     */
    ThreadPool(size_t);

    /*!
     * @brief Enqueue a new task for execution
     *
     * Adds a new task to the pool's queue. The task will be executed by
     * an available worker thread. This function is thread-safe and can
     * be called from multiple threads.
     *
     * @tparam F Callable type of the task function
     * @tparam Args Argument types for the task function
     * @param f The callable to execute
     * @param args Arguments to pass to the callable
     * @return std::future containing the result of the task
     * @throw std::runtime_error if called after the pool is stopped
     *
     * Example:
     * ```cpp
     * auto future = pool.enqueue([](int a, int b) { return a + b; }, 1, 2);
     * int result = future.get();  // result = 3
     * ```
     */
    template <class F, class... Args> auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    /*!
     * @brief Destructor - stops the pool and joins all worker threads
     *
     * Signals all worker threads to stop, waits for them to complete
     * their current tasks, and then joins all threads. This function
     * blocks until all threads have terminated.
     *
     * @note Any pending tasks in the queue will not be executed
     */
    ~ThreadPool();

  private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::queue<std::function<void()>> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
/*!
 * @brief Constructor implementation - spawns worker threads
 *
 * Creates the specified number of worker threads. Each worker runs an
 * infinite loop, waiting for tasks to become available. Workers are
 * spawned immediately and begin waiting for work.
 *
 * @param threads Number of worker threads to create
 *
 * Thread lifecycle:
 * 1. Worker thread starts
 * 2. Acquires lock on queue_mutex
 * 3. Waits on condition variable for work or stop signal
 * 4. When awakened, either exits (if stopped) or processes task
 * 5. Repeats from step 2
 */
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock,
                                         [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
}

/*!
 * @brief Enqueue a new task to the pool
 *
 * Creates a packaged_task from the provided callable and arguments,
 * then adds it to the task queue. The task will be executed by an
 * available worker thread.
 *
 * Thread safety:
 * - Acquires queue_mutex to safely add task to queue
 * - Notifies one waiting worker via condition variable
 *
 * @tparam F Callable type
 * @tparam Args Argument types
 * @param f Callable to execute
 * @param args Arguments to pass to callable
 * @return std::future containing the result once task completes
 * @throw std::runtime_error if pool has been stopped
 */
template <class F, class... Args> auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

/*!
 * @brief Destructor - stops pool and joins all worker threads
 *
 * Performs graceful shutdown of the thread pool:
 * 1. Sets stop flag to signal workers to terminate
 * 2. Notifies all workers via condition variable
 * 3. Joins each worker thread (blocks until thread completes)
 *
 * @note This blocks until all threads have terminated
 * @note Any tasks remaining in the queue will be discarded
 */
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) worker.join();
}

#endif

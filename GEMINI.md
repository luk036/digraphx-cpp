## Project Overview

This project, `digraphx-cpp`, is a C++ library for graph algorithms, with a focus on directed graphs (digraphs). It provides implementations of algorithms for finding negative cycles and solving the minimum cycle ratio problem. The project is built using modern C++20 and CMake, and it includes a comprehensive test suite using `doctest`.

The library is designed to be header-only, making it easy to integrate into other projects. It also includes a standalone executable for demonstration purposes.

### Key Technologies

*   **C++20:** The project is written in modern C++.
*   **CMake:** The project uses CMake for building and managing dependencies.
*   **doctest:** The project uses `doctest` for unit testing.
*   **xmake:** An alternative build system using `xmake` is also provided.
*   **GitHub Actions:** The project uses GitHub Actions for continuous integration.
*   **Doxygen:** The project uses Doxygen for generating documentation.

### Architecture

The project is structured as follows:

*   `include/digraphx`: Contains the header files for the library.
*   `test`: Contains the test suite for the library.
*   `standalone`: Contains a standalone executable that demonstrates the use of the library.
*   `cmake`: Contains CMake helper scripts.
*   `documentation`: Contains the Doxygen configuration and related files.

## Building and Running

The project can be built using either CMake or xmake.

### CMake

To build the project with CMake, you can use the following commands:

```bash
# Build all targets (library, tests, standalone)
cmake -S all -B build
cmake --build build

# Run the tests
./build/test/DiGraphXTests

# Run the standalone executable
./build/standalone/DiGraphX --help
```

### xmake

To build the project with xmake, you can use the following commands:

```bash
# Build the project
xmake

# Run the tests
xmake run test_digraphx
```

## Development Conventions

The project follows a number of development conventions:

*   **Coding Style:** The project uses `clang-format` to enforce a consistent coding style.
*   **Testing:** The project uses `doctest` for unit testing. All new code should be accompanied by tests.
*   **Continuous Integration:** The project uses GitHub Actions to run the tests and check the code style on every push.
*   **Documentation:** The project uses Doxygen to generate documentation from the source code.

#pragma once

#include <py2cpp/enumerate.hpp>
#include <py2cpp/range.hpp>
#include <utility>
// #include <vector>

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam Container The type of the container (e.g., std::vector)
 */
template <typename Container> class MapAdapterBase {
  protected:
    using key_type = size_t;
    using mapped_type = typename Container::value_type;
    using value_type = std::pair<key_type, mapped_type>;

    py::Range<key_type> _rng;
    Container &_lst;

    /**
     * @brief Construct a new MapAdapter object
     *
     * The function constructs a new MapAdapter object with a given container.
     *
     * @param[in] lst The parameter `lst` is a reference to a container object.
     */
    explicit MapAdapterBase(Container &lst) : _rng{py::range(lst.size())}, _lst(lst) {}

    /**
     * The function overloads the subscript operator to access and modify the value associated with
     * a given key in a map-like container.
     *
     * @param[in] key The parameter "key" is of type "key_type", which is a data type that
     * represents the key used to search for an element in the container.
     *
     * @return The operator[] is returning a reference to the mapped_type value associated with the
     * given key.
     */
    mapped_type &operator[](const key_type &key) { return this->_lst[key]; }

    /**
     * The function returns a constant reference to the value associated with the given key in a
     * map-like container.
     *
     * @param[in] key The key to access.
     *
     * @return a constant reference to the value associated with the given key in the `_lst`
     * container.
     */
    const mapped_type &operator[](const key_type &key) const { return this->_lst.at(key); }

    /**
     * The function at() returns a constant reference to the value associated with a given key in a
     * map.
     *
     * @return a constant reference to the value associated with the given key.
     */
    const mapped_type &at(const key_type &key) const { return this->_lst.at(key); }

    /**
     * The function checks if a given key is present in a data structure.
     *
     * @param[in] key The key to check for existence.
     *
     * @return a boolean value. It will return true if the key is contained in the data structure,
     * and false otherwise.
     */
    bool contains(const key_type &key) const { return this->_rng.contains(key); }

    /**
     * The size() function returns the size of the _rng container.
     *
     * @return The size of the `_rng` object is being returned.
     */
    size_t size() const { return this->_rng.size(); }
};

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam Container The type of the container (e.g., std::vector)
 */
template <typename Container> class MapAdapter : public MapAdapterBase<Container> {
  public:
    using key_type = size_t;
    using mapped_type = typename Container::value_type;
    using value_type = std::pair<key_type, mapped_type>;
    using Base = MapAdapterBase<Container>;
    using E = decltype(py::enumerate(Base::_lst));

  private:
    E mapview;

  public:
    /**
     * The function constructs a new MapAdapter object with a given container and creates a mapview
     * using py::enumerate.
     *
     * @param[in] lst The `lst` parameter is a reference to a container object.
     */
    explicit MapAdapter(Container &lst) : Base(lst), mapview(py::enumerate(this->_lst)) {}

    /**
     * The function returns an iterator pointing to the beginning of the mapview.
     *
     * @return The `begin()` function is returning an iterator pointing to the beginning of the
     * `mapview` container.
     */
    auto begin() const { return mapview.begin(); }

    /**
     * The function returns an iterator pointing to the end of the mapview.
     *
     * @return The end iterator of the mapview.
     */
    auto end() const { return mapview.end(); }
};

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam Container The type of the container (e.g., std::vector)
 */
template <typename Container> class MapConstAdapter {
  public:
    using key_type = size_t;
    using mapped_type = typename Container::value_type;
    using value_type = std::pair<key_type, mapped_type>;
    using E = decltype(py::const_enumerate(std::declval<Container>()));

  private:
    py::Range<key_type> _rng;
    const Container &_lst;
    E mapview;

  public:
    /**
     * The function constructs a MapConstAdapter object using a given container.
     *
     * @param[in] lst The parameter `lst` is a reference to a container object.
     */
    explicit MapConstAdapter(const Container &lst)
        : _rng{py::range(lst.size())}, _lst(lst), mapview(py::const_enumerate(_lst)) {}

    /**
     * The function overloads the subscript operator to access and modify the value associated with
     * a given key in a map-like container.
     *
     * @param[in] key The key to access.
     *
     * @return The operator[] is returning a reference to the mapped_type value associated with the
     * given key.
     */
    mapped_type &operator[](const key_type &key) { return this->_lst[key]; }

    /**
     * The function returns a constant reference to the value associated with the given key in a
     * map-like container.
     *
     * @param[in] key The key to access.
     *
     * @return a constant reference to the value associated with the given key in the `_lst`
     * container.
     */
    const mapped_type &operator[](const key_type &key) const { return this->_lst.at(key); }

    /**
     * The function at() returns a constant reference to the value associated with a given key in a
     * map.
     *
     * @return a constant reference to the value associated with the given key.
     */
    const mapped_type &at(const key_type &key) const { return this->_lst.at(key); }

    /**
     * The function checks if a given key is present in a data structure.
     *
     * @param[in] key The key to check for existence.
     *
     * @return a boolean value. It will return true if the key is contained in the data structure,
     * and false otherwise.
     */
    bool contains(const key_type &key) const { return this->_rng.contains(key); }

    /**
     * The size() function returns the size of the _rng container.
     *
     * @return The size of the `_rng` object is being returned.
     */
    size_t size() const { return this->_rng.size(); }

    /**
     * The function returns an iterator pointing to the beginning of the mapview.
     *
     * @return The `begin()` function is returning an iterator pointing to the beginning of the
     * `mapview` container.
     */
    auto begin() const { return mapview.begin(); }

    /**
     * The function returns an iterator pointing to the end of the mapview.
     *
     * @return The end iterator of the mapview.
     */
    auto end() const { return mapview.end(); }
};

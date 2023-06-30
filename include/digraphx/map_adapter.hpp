#pragma once

#include <py2cpp/enumerate.hpp>
#include <py2cpp/range.hpp>
#include <utility>
#include <vector>

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam T
 */
template <typename Container> class MapAdapterBase {
public:
  using key_type = size_t;
  using mapped_type = Container::value_type;
  using value_type = std::pair<key_type, mapped_type>;

  py::Range<key_type> _rng;
  Container &_lst;

public:
  /**
   * @brief Construct a new MapAdapter object
   *
   * @param lst
   */
  explicit MapAdapterBase(Container &lst)
      : _rng{py::range(lst.size())}, _lst(lst) {}

  /**
   * @brief
   *
   * @param key
   * @return T&
   */
  mapped_type &operator[](const key_type &key) { return this->_lst[key]; }

  /**
   * @brief
   *
   * @param key
   * @return const T&
   */
  const mapped_type &operator[](const key_type &key) const {
    return this->_lst.at(key);
  }

  /**
   * @brief
   *
   * @param key
   * @return const T&
   */
  const mapped_type &at(const key_type &key) const {
    return this->_lst.at(key);
  }

  // void erase() { throw std::runtime_error("NotImplementedError"); }

  /**
   * @brief
   *
   * @param key
   * @return true
   * @return false
   */
  bool contains(const key_type &key) const { return this->_rng.contains(key); }

  /**
   * @brief
   *
   * @return size_t
   */
  size_t size() const { return this->_rng.size(); }
};

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam T
 */
template <typename Container>
class MapAdapter : public MapAdapterBase<Container> {
public:
  using key_type = size_t;
  using mapped_type = Container::value_type;
  using value_type = std::pair<key_type, mapped_type>;
  using Base = MapAdapterBase<Container>;
  using E = decltype(py::enumerate(Base::_lst));

private:
  E mapview;

public:
  /**
   * @brief Construct a new MapAdapter object
   *
   * @param lst
   */
  explicit MapAdapter(Container &lst)
      : Base(std::forward<Container>(lst)), mapview(py::enumerate(this->_lst)) {
  }

  auto begin() const { return mapview.begin(); }
  auto end() const { return mapview.end(); }
};

/**
 * @brief Dict-like data structure by std::vector and Range
 *
 * @tparam T
 */
template <typename Container> class MapConstAdapter {
public:
  using key_type = size_t;
  using mapped_type = Container::value_type;
  using value_type = std::pair<key_type, mapped_type>;
  using E = decltype(py::const_enumerate(std::declval<Container>()));

private:
  py::Range<key_type> _rng;
  const Container &_lst;
  E mapview;

public:
  /**
   * @brief Construct a new MapAdapter object
   *
   * @param lst
   */
  explicit MapConstAdapter(const Container &lst)
      : _rng{py::range(lst.size())}, _lst(lst),
        mapview(py::const_enumerate(_lst)) {}

  /**
   * @brief
   *
   * @param key
   * @return T&
   */
  mapped_type &operator[](const key_type &key) { return this->_lst[key]; }

  /**
   * @brief
   *
   * @param key
   * @return const T&
   */
  const mapped_type &operator[](const key_type &key) const {
    return this->_lst.at(key);
  }

  /**
   * @brief
   *
   * @param key
   * @return const T&
   */
  const mapped_type &at(const key_type &key) const {
    return this->_lst.at(key);
  }

  // void erase() { throw std::runtime_error("NotImplementedError"); }

  /**
   * @brief
   *
   * @param key
   * @return true
   * @return false
   */
  bool contains(const key_type &key) const { return this->_rng.contains(key); }

  /**
   * @brief
   *
   * @return size_t
   */
  size_t size() const { return this->_rng.size(); }

  auto begin() const { return mapview.begin(); }
  auto end() const { return mapview.end(); }
};

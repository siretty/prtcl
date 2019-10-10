#pragma once

#include "host_linear_buffer.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

namespace prtcl {

template <typename T> class host_linear_access {
  size_t size_ = 0;
  T *data_ = nullptr;

public:
  host_linear_access() = default;
  host_linear_access(host_linear_access const &) = default;
  host_linear_access &operator=(host_linear_access const &) = default;
  host_linear_access(host_linear_access &&) = default;
  host_linear_access &operator=(host_linear_access &&) = default;

  // private:
  host_linear_access(size_t s, T *d) : size_{s}, data_{d} {}

public:
  size_t size() const { return size_; }

  T *data() const { return data_; }

public:
  T *begin() const { return data(); }

  T *end() const { return data() + size(); }

public:
  template <typename U = T,
            typename = std::enable_if_t<!std::is_const<U>::value>>
  T &operator[](size_t pos) const {
    return data_[pos];
  }

  template <typename U = T,
            typename = std::enable_if_t<std::is_const<U>::value>>
  T const operator[](size_t pos) const {
    return data_[pos];
  }
};

namespace result_of {

// Primary template is never defined.
template <typename...> struct get_rw_access;

template <typename T> struct get_rw_access<host_linear_buffer<T>> {
  using type = host_linear_access<T>;
};

} // namespace result_of

template <typename T>
typename result_of::get_rw_access<host_linear_buffer<T>>::type
get_rw_access(host_linear_buffer<T> const &buffer) {
  static_assert(!std::is_const<T>::value);
  auto *data = detail::host_linear_buffer_access::data(buffer);
  return {buffer.size(), data};
}

namespace result_of {

// Primary template is never defined.
template <typename, typename...> struct get_ro_access;

template <typename T> struct get_ro_access<host_linear_buffer<T>> {
  using type = host_linear_access<T const>;
};

} // namespace result_of

template <typename T>
typename result_of::get_ro_access<host_linear_buffer<T>>::type
get_ro_access(host_linear_buffer<T> const &buffer) {
  static_assert(!std::is_const<T>::value);
  auto *data = detail::host_linear_buffer_access::data(buffer);
  return {buffer.size(), data};
}

} // namespace prtcl

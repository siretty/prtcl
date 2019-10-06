#pragma once

#include "host_linear_data.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

namespace prtcl {

template <typename T> struct host_linear_access {
  size_t size_ = 0;
  T *data_ = nullptr;

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

template <typename T>
host_linear_access<T> get_buffer(host_linear_data<T> const &data) {
  return host_linear_access<T>{data.size(), data.data()};
}

template <typename T>
host_linear_access<T> get_rw_access(host_linear_access<T> const &buffer) {
  static_assert(!std::is_const<T>::value);
  return host_linear_access<T>{buffer.size(), buffer.data()};
}

template <typename T>
host_linear_access<T const> get_ro_access(host_linear_access<T> const &buffer) {
  static_assert(!std::is_const<T>::value);
  return host_linear_access<T const>{buffer.size(), buffer.data()};
}

} // namespace prtcl

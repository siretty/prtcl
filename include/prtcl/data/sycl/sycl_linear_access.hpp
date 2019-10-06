#pragma once

#include "../../tags/sycl.hpp"
#include "sycl_linear_buffer.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

#include "../../libs/sycl.hpp"

namespace prtcl {

template <typename T, typename SYCLAccessor> struct sycl_linear_access {
  SYCLAccessor data_;

public:
  size_t size() const { return data_.get_range()[0]; }

  auto data() const { return data_.get_pointer(); }

public:
  auto begin() const { return data(); }

  auto end() const { return data() + size(); }

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

template <typename T, typename... Args>
auto get_rw_access(sycl_linear_buffer<T> &buffer, Args &&... args) {
  static_assert(!std::is_const<T>::value);
  constexpr sycl::access::mode Mode = sycl::access::mode::read_write;
  using accessor_type = decltype(
      buffer.data_.template get_access<Mode>(std::forward<Args>(args)...));
  return sycl_linear_access<T, accessor_type>{
      buffer.data_.template get_access<Mode>(std::forward<Args>(args)...)};
}

template <typename T, typename... Args>
auto get_ro_access(sycl_linear_buffer<T> &buffer, Args &&... args) {
  static_assert(!std::is_const<T>::value);
  constexpr sycl::access::mode Mode = sycl::access::mode::read;
  using accessor_type = decltype(
      buffer.data_.template get_access<Mode>(std::forward<Args>(args)...));
  return sycl_linear_access<T const, accessor_type>{
      buffer.data_.template get_access<Mode>(std::forward<Args>(args)...)};
}

} // namespace prtcl

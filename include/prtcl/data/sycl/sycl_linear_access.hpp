#pragma once

#include "../../meta/remove_cvref.hpp"
#include "../host/host_linear_access.hpp"
#include "../host/host_linear_buffer.hpp"
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

namespace result_of {

template <typename T, typename... Args>
struct get_rw_access<sycl_linear_buffer<T>, Args...> {
  using accessor_type = remove_cvref_t<decltype(
      detail::sycl_linear_buffer_access::sycl_buffer(
          std::declval<sycl_linear_buffer<T>>())
          .template get_access<sycl::access::mode::read_write>(
              std::declval<Args>()...))>;
  using type = sycl_linear_access<T, accessor_type>;
};

} // namespace result_of

template <typename T, typename... Args>
typename result_of::get_rw_access<sycl_linear_buffer<T>, Args...>::type
get_rw_access(sycl_linear_buffer<T> buffer, Args &&... args) {
  static_assert(!std::is_const<T>::value);
  return {detail::sycl_linear_buffer_access::sycl_buffer(buffer)
              .template get_access<sycl::access::mode::read_write>(
                  std::forward<Args>(args)...)};
}

namespace result_of {

template <typename T, typename... Args>
struct get_ro_access<sycl_linear_buffer<T>, Args...> {
  using accessor_type = remove_cvref_t<decltype(
      detail::sycl_linear_buffer_access::sycl_buffer(
          std::declval<sycl_linear_buffer<T>>())
          .template get_access<sycl::access::mode::read>(
              std::declval<Args>()...))>;
  using type = sycl_linear_access<T const, accessor_type>;
};

} // namespace result_of

template <typename T, typename... Args>
typename result_of::get_ro_access<sycl_linear_buffer<T>, Args...>::type
get_ro_access(sycl_linear_buffer<T> buffer, Args &&... args) {
  static_assert(!std::is_const<T>::value);
  return {detail::sycl_linear_buffer_access::sycl_buffer(buffer)
              .template get_access<sycl::access::mode::read>(
                  std::forward<Args>(args)...)};
}

} // namespace prtcl

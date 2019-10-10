#pragma once

#include "../../tags.hpp"
#include "../host/host_linear_buffer.hpp" // result_of::get_buffer
#include "../host/host_linear_data.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

#include "../../libs/sycl.hpp"

namespace prtcl {

namespace detail {
struct sycl_linear_buffer_access;
} // namespace detail

template <typename T> struct sycl_linear_buffer {
  sycl::buffer<T, 1> data_;

public:
  sycl_linear_buffer() = default;
  sycl_linear_buffer(sycl_linear_buffer const &) = default;
  sycl_linear_buffer &operator=(sycl_linear_buffer const &) = default;
  sycl_linear_buffer(sycl_linear_buffer &&) = default;
  sycl_linear_buffer &operator=(sycl_linear_buffer &&) = default;

private:
  sycl_linear_buffer(size_t s, T *d) : data_{d, sycl::range<1>{s}} {}

public:
  size_t size() const { return data_.get_range()[0]; }

  friend struct detail::sycl_linear_buffer_access;
};

namespace detail {

struct sycl_linear_buffer_access {
  template <typename T> static sycl_linear_buffer<T> make(size_t s, T *d) {
    return {s, d};
  }

  template <typename T>
  static sycl::buffer<T, 1> sycl_buffer(sycl_linear_buffer<T> const &b) {
    return b.data_;
  }
};

} // namespace detail

namespace result_of {

template <typename T> struct get_buffer<host_linear_data<T>, tag::sycl> {
  using type = sycl_linear_buffer<T>;
};

} // namespace result_of

template <typename T>
typename result_of::get_buffer<host_linear_data<T>, tag::sycl>::type
get_buffer(host_linear_data<T> const &data, tag::sycl) {
  return detail::sycl_linear_buffer_access::make(data.size(), data.data());
}

} // namespace prtcl

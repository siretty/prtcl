#pragma once

#include "../../tags.hpp"
#include "host_linear_data.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

namespace prtcl {

namespace detail {
struct host_linear_buffer_access;
} // namespace detail

template <typename T> class host_linear_buffer {
  size_t size_ = 0;
  T *data_ = nullptr;

public:
  host_linear_buffer() = default;
  host_linear_buffer(host_linear_buffer const &) = default;
  host_linear_buffer &operator=(host_linear_buffer const &) = default;
  host_linear_buffer(host_linear_buffer &&) = default;
  host_linear_buffer &operator=(host_linear_buffer &&) = default;

  // private:
  host_linear_buffer(size_t s, T *d) : size_{s}, data_{d} {}

public:
  size_t size() const { return size_; }

  friend struct detail::host_linear_buffer_access;
};

namespace detail {

struct host_linear_buffer_access {
  template <typename T> static T *data(host_linear_buffer<T> const &b) {
    return b.data_;
  }
};

} // namespace detail

namespace result_of {

// Primary template is never defined.
template <typename, typename...> struct get_buffer;

/// Result type of get_buffer(host_linear_data<T>).
template <typename T> struct get_buffer<host_linear_data<T>, tag::host> {
  using type = host_linear_buffer<T>;
};

} // namespace result_of

template <typename T>
typename result_of::get_buffer<host_linear_data<T>, tag::host>::type
get_buffer(host_linear_data<T> const &data, tag::host) {
  return {data.size(), data.data()};
}

} // namespace prtcl

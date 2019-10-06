#pragma once

#include "../../tags/sycl.hpp"
#include "../host/host_linear_data.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

#include "../../libs/sycl.hpp"

namespace prtcl {

template <typename T> struct sycl_linear_buffer {
  sycl::buffer<T, 1> data_;

public:
  size_t size() const { return data_.get_range()[0]; }
};

template <typename T, typename... Args>
sycl_linear_buffer<T> get_buffer(host_linear_data<T> const &data, tags::sycl,
                                 Args &&... args) {
  return sycl_linear_buffer<T>{sycl::buffer<T, 1>{
      data.data(), sycl::range<1>{data.size()}, std::forward<Args>(args)...}};
}

} // namespace prtcl

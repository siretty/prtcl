#pragma once

#include <prtcl/meta/unpack_integer_sequence.hpp>

#include <array>
#include <utility>
#include <vector>

#include <cstddef>

namespace prtcl::detail {

struct component_data_access {
  template <typename Self, typename... Args>
  static auto component_data(Self &&self_, Args &&... args_) {
    return std::forward<Self>(self_).component_data(
        std::forward<Args>(args_)...);
  }
};

struct resize_access {
  template <typename Self, typename... Args>
  static auto resize(Self &&self_, Args &&... args_) {
    return std::forward<Self>(self_).resize(std::forward<Args>(args_)...);
  }

  template <typename Self, typename... Args>
  static auto clear(Self &&self_, Args &&... args_) {
    return std::forward<Self>(self_).clear(std::forward<Args>(args_)...);
  }
};

} // namespace prtcl::detail

namespace prtcl::data {

template <typename Scalar, typename Shape> class tensors {
public:
  using scalar_type = Scalar;
  using shape_type = Shape;

private:
  using linear_storage = std::vector<scalar_type>;

public:
  static constexpr size_t rank() {
    return meta::unpack_integer_sequence(
        [](auto... args) { return sizeof...(args); }, shape_type{});
  }

  static constexpr size_t component_count() {
    return meta::unpack_integer_sequence(
        [](auto... args) { return (args * ... * 1); }, shape_type{});
  }

public:
  size_t capacity() const { return _component_stride; }

  void reserve(size_t new_capacity_) {
    new_capacity_ = (0 == new_capacity_ ? 1 : new_capacity_);

    // TODO: increase new_capacity to be divisible by some general alignment

    if (_component_stride >= new_capacity_)
      return;

    // create new storage and copy all values
    linear_storage new_linear(component_count() * new_capacity_);

    // copy all old values into the new linear storage
    if (0 < _linear.size()) {
      for (size_t n = 0; n < component_count(); ++n) {
        std::copy_n(_linear.data() + n * _component_stride, _size,
                    new_linear.data() + n * new_capacity_);
      }
    }

    _linear = std::move(new_linear);
    _component_stride = new_capacity_;
  }

public:
  size_t size() const { return _size; }

private:
  void resize(size_t new_size_) {
    // ensure that enough memory is available
    this->reserve(new_size_);

    _size = new_size_;
  }

  void clear() { _size = 0; }

  friend ::prtcl::detail::resize_access;

private:
  scalar_type *component_data(size_t index_) {
    return _linear.data() + index_ * _component_stride;
  }

  scalar_type const *component_data(size_t index_) const {
    return _linear.data() + index_ * _component_stride;
  }

  friend ::prtcl::detail::component_data_access;

private:
  size_t _component_stride = 1;
  size_t _size = 0;
  linear_storage _linear;

  static_assert(0 <= rank() && rank() <= 2);
};

template <typename T, size_t... Ns>
using tensors_t = tensors<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

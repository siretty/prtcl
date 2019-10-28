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
  size_t capacity() const { return _linear[0].capacity(); }

  void reserve(size_t capacity_) {
    for (auto &l : _linear)
      l.reserve(capacity_);
  }

public:
  size_t size() const { return _linear[0].size(); }

private:
  void resize(size_t size_) {
    for (auto &l : _linear)
      l.resize(size_);
  }

  void clear() {
    for (auto &l : _linear)
      l.clear();
  }

  friend ::prtcl::detail::resize_access;

private:
  scalar_type *component_data(size_t index_) { return _linear[index_].data(); }

  scalar_type const *component_data(size_t index_) const {
    return _linear[index_].data();
  }

  friend ::prtcl::detail::component_data_access;

private:
  std::array<linear_storage, component_count()> _linear;

  static_assert(0 <= rank() && rank() <= 2);
};

template <typename T, size_t... Ns>
using tensors_t = tensors<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

#pragma once

#include <prtcl/data/tensors.hpp>
#include <prtcl/meta/compute_block_index.hpp>
#include <prtcl/meta/unpack_integer_sequence.hpp>

#include <array>

#include <cstddef>
#include <utility>

namespace prtcl::data::openmp {

template <typename Scalar, typename Shape> class tensors {
public:
  using scalar_type = Scalar;
  using shape_type = Shape;

public:
  tensors() = delete;

  tensors(tensors const &) = default;
  tensors &operator=(tensors const &) = default;

  tensors(tensors &&) = default;
  tensors &operator=(tensors &&) = default;

  explicit tensors(::prtcl::data::tensors<Scalar, Shape> &from_)
      : _data{}, _size{from_.size()}, _capacity{from_.capacity()} {
    for (size_t n = 0; n < component_count(); ++n)
      _data[n] = detail::component_data_access::component_data(from_, n);
  }

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
  size_t capacity() const { return _capacity; }

public:
  size_t size() const { return _size; }

public:
  template <typename MultiIndex>
  scalar_type *component_data(MultiIndex &&mi_) const {
    return _data[meta::linear_block_index(shape_type{},
                                          std::forward<MultiIndex>(mi_))];
  }

private:
  std::array<scalar_type *, component_count()> _data = {};
  size_t _size = 0;
  size_t _capacity = 0;
};

template <typename Scalar, typename Shape>
tensors(::prtcl::data::tensors<Scalar, Shape> &)->tensors<Scalar, Shape>;

} // namespace prtcl::data::openmp

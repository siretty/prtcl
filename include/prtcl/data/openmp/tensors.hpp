#pragma once

#include <prtcl/data/tensors.hpp>
#include <prtcl/meta/compute_block_index.hpp>
#include <prtcl/meta/get.hpp>
#include <prtcl/meta/unpack_integer_sequence.hpp>

#include <array>

#include <cstddef>
#include <utility>

#include <Eigen/Eigen>

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
      : _component_stride{from_.capacity()}, _size{from_.size()}, _data{} {
    _data = detail::component_data_access::component_data(from_, 0ul);
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
  size_t capacity() const { return _component_stride; }

public:
  size_t size() const { return _size; }

public:
  template <typename MultiIndex>
  scalar_type *component_data(MultiIndex &&mi_) const {
    return _data + meta::linear_block_index(shape_type{},
                                            std::forward<MultiIndex>(mi_)) *
                       _component_stride;
  }

private:
  template <typename S> struct eigen_matrix;

  template <typename I, I Rows>
  struct eigen_matrix<std::integer_sequence<I, Rows>> {
    using type = Eigen::Matrix<scalar_type, static_cast<int>(Rows), 1>;
  };

  template <typename I, I Rows, I Cols>
  struct eigen_matrix<std::integer_sequence<I, Rows, Cols>> {
    using type = Eigen::Matrix<scalar_type, static_cast<int>(Rows),
                               static_cast<int>(Cols)>;
  };

public:
  decltype(auto) operator[](size_t index_) const {
    if constexpr (0 == rank())
      return _data[index_];
    else {
      // TODO: implement strides more generically (to allow for easier changes
      //       in the tensor implementation wrt. C/Fortran ordering and storing
      //       contiguous component vs. contiguous elements)
      using eigen_map =
          Eigen::Map<typename eigen_matrix<Shape>::type, 0,
                     Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>;
      if constexpr (1 == rank())
        return eigen_map{_data + index_,
                         Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>{
                             static_cast<int>(_component_stride),
                             static_cast<int>(_component_stride)}};
      else if constexpr (2 == rank())
        return eigen_map{_data + index_,
                         Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>{
                             static_cast<int>(meta::get<1>(shape_type{}) *
                                              _component_stride),
                             static_cast<int>(_component_stride)}};
    }
    throw "unsupported rank";
  }

private:
  size_t _component_stride = 0;
  size_t _size = 0;
  scalar_type *_data = nullptr;
};

template <typename Scalar, typename Shape>
tensors(::prtcl::data::tensors<Scalar, Shape> &)->tensors<Scalar, Shape>;

template <typename T, size_t... Ns>
using tensors_t = tensors<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data::openmp

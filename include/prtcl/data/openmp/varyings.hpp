#pragma once

#include <prtcl/data/openmp/tensors.hpp>

#include <utility>
#include <vector>

namespace prtcl::data::openmp {

template <typename Scalar, typename Shape> class varyings {
public:
  using data_type = ::prtcl::data::openmp::tensors<Scalar, Shape>;

public:
  size_t size() const { return _size; }

public:
  auto const &operator[](size_t field_) const { return _i2d[field_]; }

private:
  size_t _size = 0;
  std::vector<data_type> _i2d;
};

template <typename T, size_t... Ns>
using varyings_t = varyings<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data::openmp

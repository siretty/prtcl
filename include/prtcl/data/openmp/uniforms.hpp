#pragma once

#include <prtcl/data/openmp/tensors.hpp>

#include <string>
#include <unordered_map>
#include <utility>

namespace prtcl::data::openmp {

template <typename Scalar, typename Shape> class uniforms {
public:
  using data_type = ::prtcl::data::openmp::tensors<Scalar, Shape>;

public:
  // TODO: INCORRECT
  auto const &operator[](size_t /*field_*/) const { return _i2d; }

private:
  data_type _i2d;
};

template <typename T, size_t... Ns>
using uniforms_t = uniforms<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data::openmp

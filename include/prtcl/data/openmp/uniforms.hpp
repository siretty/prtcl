#pragma once

#include <prtcl/data/openmp/tensors.hpp>
#include <prtcl/data/uniforms.hpp>

#include <string>
#include <unordered_map>
#include <utility>

namespace prtcl::data::openmp {

template <typename Scalar, typename Shape> class uniforms {
public:
  using data_type = ::prtcl::data::openmp::tensors<Scalar, Shape>;

public:
  uniforms() = delete;

  uniforms(uniforms const &) = default;
  uniforms &operator=(uniforms const &) = default;

  uniforms(uniforms &&) = default;
  uniforms &operator=(uniforms &&) = default;

  explicit uniforms(::prtcl::data::uniforms<Scalar, Shape> &from_)
      : _i2d{::prtcl::detail::uniforms_access::i2d(from_)} {}

public:
  size_t field_count() const { return _i2d.size(); }

public:
  // TODO: INCORRECT
  auto const &operator[](size_t /*field_*/) const { return _i2d; }

private:
  data_type _i2d;
};

template <typename Scalar, typename Shape>
uniforms(::prtcl::data::uniforms<Scalar, Shape> &)->uniforms<Scalar, Shape>;

template <typename T, size_t... Ns>
using uniforms_t = uniforms<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data::openmp

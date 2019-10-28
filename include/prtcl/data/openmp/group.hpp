#pragma once

#include <prtcl/data/openmp/uniforms.hpp>
#include <prtcl/data/openmp/varyings.hpp>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class group {
public:
  size_t size() const { return _size; }

public:
  auto const &get_uniform_scalar(size_t field_) const { return _us[field_]; }
  auto const &get_uniform_vector(size_t field_) const { return _uv[field_]; }
  auto const &get_uniform_matrix(size_t field_) const { return _um[field_]; }

  auto const &get_varying_scalar(size_t field_) const { return _vs[field_]; }
  auto const &get_varying_vector(size_t field_) const { return _vv[field_]; }
  auto const &get_varying_matrix(size_t field_) const { return _vm[field_]; }

private:
  size_t _size = 0;

  uniforms_t<Scalar> _us;
  uniforms_t<Scalar, N> _uv;
  uniforms_t<Scalar, N, N> _um;

  varyings_t<Scalar> _vs;
  varyings_t<Scalar, N> _vv;
  varyings_t<Scalar, N, N> _vm;
};

} // namespace prtcl::data::openmp

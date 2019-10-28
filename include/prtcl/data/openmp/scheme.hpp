#pragma once

#include <prtcl/data/openmp/group.hpp>
#include <prtcl/data/openmp/uniforms.hpp>

#include <vector>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class scheme {
public:
  auto const &get_global_scalar(size_t field_) const { return _gs[field_]; }
  auto const &get_global_vector(size_t field_) const { return _gv[field_]; }
  auto const &get_global_matrix(size_t field_) const { return _gm[field_]; }

public:
  auto const &get_group(size_t group_) const { return _i2d[group_]; }

private:
  uniforms_t<Scalar> _gs;
  uniforms_t<Scalar, N> _gv;
  uniforms_t<Scalar, N, N> _gm;

  std::vector<group<Scalar, N>> _i2d;
};

} // namespace prtcl::data::openmp

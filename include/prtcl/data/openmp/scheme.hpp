#pragma once

#include <prtcl/data/openmp/group.hpp>
#include <prtcl/data/openmp/uniforms.hpp>
#include <prtcl/data/scheme.hpp>

#include <vector>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class scheme {
public:
  scheme() = delete;

  scheme(scheme const &) = default;
  scheme &operator=(scheme const &) = default;

  scheme(scheme &&) = default;
  scheme &operator=(scheme &&) = default;

  explicit scheme(::prtcl::data::scheme<Scalar, N> &from_)
      : _gs{::prtcl::detail::scheme_access::gs(from_)},
        _gv{::prtcl::detail::scheme_access::gv(from_)},
        _gm{::prtcl::detail::scheme_access::gm(from_)} {
    auto &from_i2d = ::prtcl::detail::scheme_access::i2d(from_);
    _i2d.reserve(from_i2d.size());
    for (size_t group = 0; group < from_i2d.size(); ++group)
      _i2d.emplace_back(from_i2d[group]);
  }

public:
  size_t get_global_scalar_count() const { return _gs.field_count(); }
  size_t get_global_vector_count() const { return _gv.field_count(); }
  size_t get_global_matrix_count() const { return _gm.field_count(); }

  auto const &get_global_scalar(size_t field_) const { return _gs[field_]; }
  auto const &get_global_vector(size_t field_) const { return _gv[field_]; }
  auto const &get_global_matrix(size_t field_) const { return _gm[field_]; }

public:
  size_t get_group_count() const { return _i2d.size(); }

  auto const &get_group(size_t group_) const { return _i2d[group_]; }

private:
  uniforms_t<Scalar> _gs;
  uniforms_t<Scalar, N> _gv;
  uniforms_t<Scalar, N, N> _gm;

  std::vector<group<Scalar, N>> _i2d;
};

template <typename Scalar, size_t N>
scheme(::prtcl::data::scheme<Scalar, N> &)->scheme<Scalar, N>;

} // namespace prtcl::data::openmp

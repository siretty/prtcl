#pragma once

#include <prtcl/data/group.hpp>
#include <prtcl/data/openmp/uniforms.hpp>
#include <prtcl/data/openmp/varyings.hpp>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class group {
public:
  group() = delete;

  group(group const &) = default;
  group &operator=(group const &) = default;

  group(group &&) = default;
  group &operator=(group &&) = default;

  explicit group(::prtcl::data::group<Scalar, N> &from_)
      : _size{from_.size()}, _us{::prtcl::detail::group_access::us(from_)},
        _uv{::prtcl::detail::group_access::uv(from_)},
        _um{::prtcl::detail::group_access::um(from_)},
        _vs{::prtcl::detail::group_access::vs(from_)},
        _vv{::prtcl::detail::group_access::vv(from_)},
        _vm{::prtcl::detail::group_access::vm(from_)} {}

public:
  size_t size() const { return _size; }

public:
  size_t get_uniform_scalar_count() const { return _us.field_count(); }
  size_t get_uniform_vector_count() const { return _uv.field_count(); }
  size_t get_uniform_matrix_count() const { return _um.field_count(); }

  size_t get_varying_scalar_count() const { return _vs.field_count(); }
  size_t get_varying_vector_count() const { return _vv.field_count(); }
  size_t get_varying_matrix_count() const { return _vm.field_count(); }

  auto const &get_uniform_scalar(size_t field_) const { return _us[field_]; }
  auto const &get_uniform_vector(size_t field_) const { return _uv[field_]; }
  auto const &get_uniform_matrix(size_t field_) const { return _um[field_]; }

  auto const &get_varying_scalar(size_t field_) const { return _vs[field_]; }
  auto const &get_varying_vector(size_t field_) const { return _vv[field_]; }
  auto const &get_varying_matrix(size_t field_) const { return _vm[field_]; }

  // TODO: is this better than the individual methods above? rethink when used /
  //       unused in expr transformations

  // get(tag::[uniform,varying], tag::[scalar,vector,matrix]) [const] -> ... {{{

  auto &get(tag::uniform, tag::scalar) { return _us; }
  auto &get(tag::uniform, tag::scalar) const { return _us; }

  auto &get(tag::uniform, tag::vector) { return _uv; }
  auto &get(tag::uniform, tag::vector) const { return _uv; }

  auto &get(tag::uniform, tag::matrix) { return _um; }
  auto &get(tag::uniform, tag::matrix) const { return _um; }

  auto &get(tag::varying, tag::scalar) { return _vs; }
  auto &get(tag::varying, tag::scalar) const { return _vs; }

  auto &get(tag::varying, tag::vector) { return _vv; }
  auto &get(tag::varying, tag::vector) const { return _vv; }

  auto &get(tag::varying, tag::matrix) { return _vm; }
  auto &get(tag::varying, tag::matrix) const { return _vm; }

  // }}}

private:
  size_t _size = 0;

  uniforms_t<Scalar> _us;
  uniforms_t<Scalar, N> _uv;
  uniforms_t<Scalar, N, N> _um;

  varyings_t<Scalar> _vs;
  varyings_t<Scalar, N> _vv;
  varyings_t<Scalar, N, N> _vm;
};

template <typename Scalar, size_t N>
group(::prtcl::data::group<Scalar, N> &)->group<Scalar, N>;

} // namespace prtcl::data::openmp

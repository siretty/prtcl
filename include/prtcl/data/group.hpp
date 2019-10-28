#pragma once

#include "prtcl/data/tensors.hpp"
#include <prtcl/data/uniforms.hpp>
#include <prtcl/data/varyings.hpp>

namespace prtcl::data {

template <typename Scalar, size_t N> class group {
public:
  size_t size() const { return _size; }

  void resize(size_t new_size_) {
    ::prtcl::detail::resize_access::resize(_vs, new_size_);
    ::prtcl::detail::resize_access::resize(_vv, new_size_);
    ::prtcl::detail::resize_access::resize(_vm, new_size_);

    _size = new_size_;
  }

public:
  bool has_uniform_scalar(std::string name_) const { return _us.has(name_); }
  void add_uniform_scalar(std::string name_) { _us.add(name_); }

  bool has_uniform_vector(std::string name_) const { return _uv.has(name_); }
  void add_uniform_vector(std::string name_) { _uv.add(name_); }

  bool has_uniform_matrix(std::string name_) const { return _um.has(name_); }
  void add_uniform_matrix(std::string name_) { _um.add(name_); }

  bool has_varying_scalar(std::string name_) const { return _vs.has(name_); }
  auto &add_varying_scalar(std::string name_) { return _vs.add(name_); }

  bool has_varying_vector(std::string name_) const { return _vv.has(name_); }
  auto &add_varying_vector(std::string name_) { return _vv.add(name_); }

  bool has_varying_matrix(std::string name_) const { return _vm.has(name_); }
  auto &add_varying_matrix(std::string name_) { return _vm.add(name_); }

private:
  size_t _size = 0;

  uniforms_t<Scalar> _us;
  uniforms_t<Scalar, N> _uv;
  uniforms_t<Scalar, N, N> _um;

  varyings_t<Scalar> _vs;
  varyings_t<Scalar, N> _vv;
  varyings_t<Scalar, N, N> _vm;
};

} // namespace prtcl::data

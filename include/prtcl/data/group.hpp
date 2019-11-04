#pragma once

#include <prtcl/data/uniforms.hpp>
#include <prtcl/data/varyings.hpp>
#include <prtcl/tags.hpp>

#include <unordered_set>

namespace prtcl::detail {

struct group_access {
  template <typename Self, typename... Args> static auto &us(Self &&self_) {
    return std::forward<Self>(self_)._us;
  }

  template <typename Self, typename... Args> static auto &uv(Self &&self_) {
    return std::forward<Self>(self_)._uv;
  }

  template <typename Self, typename... Args> static auto &um(Self &&self_) {
    return std::forward<Self>(self_)._um;
  }

  template <typename Self, typename... Args> static auto &vs(Self &&self_) {
    return std::forward<Self>(self_)._vs;
  }

  template <typename Self, typename... Args> static auto &vv(Self &&self_) {
    return std::forward<Self>(self_)._vv;
  }

  template <typename Self, typename... Args> static auto &vm(Self &&self_) {
    return std::forward<Self>(self_)._vm;
  }
};

} // namespace prtcl::detail

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

  void add_flag(std::string flag_) { _flags.insert(flag_); }

  bool has_flag(std::string flag_) const {
    return _flags.find(flag_) != _flags.end();
  }

private:
  size_t _size = 0;

  uniforms_t<Scalar> _us;
  uniforms_t<Scalar, N> _uv;
  uniforms_t<Scalar, N, N> _um;

  varyings_t<Scalar> _vs;
  varyings_t<Scalar, N> _vv;
  varyings_t<Scalar, N, N> _vm;

  std::unordered_set<std::string> _flags;

  friend ::prtcl::detail::group_access;
};

} // namespace prtcl::data

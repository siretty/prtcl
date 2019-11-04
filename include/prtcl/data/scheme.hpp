#pragma once

#include <prtcl/data/group.hpp>
#include <prtcl/data/uniforms.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace prtcl::detail {

struct scheme_access {
  template <typename Self, typename... Args> static auto &i2d(Self &self_) {
    return self_._i2d;
  }
};

} // namespace prtcl::detail

namespace prtcl::data {

template <typename Scalar, size_t N> class scheme {
public:
  bool has_global_scalar(std::string name_) const { return _gs.has(name_); }
  void add_global_scalar(std::string name_) { _gs.add(name_); }

  bool has_global_vector(std::string name_) const { return _gv.has(name_); }
  void add_global_vector(std::string name_) { _gv.add(name_); }

  bool has_global_matrix(std::string name_) const { return _gm.has(name_); }
  void add_global_matrix(std::string name_) { _gm.add(name_); }

  // get(tag::global, tag::[scalar,vector,matrix]) [const] -> ... {{{

  auto &get(tag::global, tag::scalar) { return _gs; }
  auto &get(tag::global, tag::scalar) const { return _gs; }

  auto &get(tag::global, tag::vector) { return _gv; }
  auto &get(tag::global, tag::vector) const { return _gv; }

  auto &get(tag::global, tag::matrix) { return _gm; }
  auto &get(tag::global, tag::matrix) const { return _gm; }

  // }}}

public:
  bool has_group(std::string name_) const {
    return _n2i.find(name_) != _n2i.end();
  }

  auto &add_group(std::string name_) {
    size_t index = _i2d.size();
    auto [it, inserted] = _n2i.insert({name_, index});
    if (inserted) {
      _i2d.resize(index + 1);
      _i2d[index] = std::make_unique<group<Scalar, N>>();
    }
    return *_i2d[it->second];
  }

  size_t get_group_count() const { return _i2d.size(); }

  std::optional<size_t> get_group_index(std::string name_) {
    if (auto it = _n2i.find(name_); it != name_)
      return it->second;
    else
      return std::nullopt;
  }

  auto &get_group(size_t index_) { return *_i2d[index_]; }
  auto &get_group(size_t index_) const { return *_i2d[index_]; }

private:
  uniforms_t<Scalar> _gs;
  uniforms_t<Scalar, N> _gv;
  uniforms_t<Scalar, N, N> _gm;

  std::unordered_map<std::string, size_t> _n2i;
  std::vector<std::unique_ptr<group<Scalar, N>>> _i2d;

  friend ::prtcl::detail::scheme_access;
};

} // namespace prtcl::data

#pragma once

#include <prtcl/data/group.hpp>
#include <prtcl/data/uniforms.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace prtcl::data {

template <typename Scalar, size_t N> class scheme {
public:
  bool has_global_scalar(std::string name_) const { return _gs.has(name_); }
  void add_global_scalar(std::string name_) { _gs.add(name_); }

  bool has_global_vector(std::string name_) const { return _gv.has(name_); }
  void add_global_vector(std::string name_) { _gv.add(name_); }

  bool has_global_matrix(std::string name_) const { return _gm.has(name_); }
  void add_global_matrix(std::string name_) { _gm.add(name_); }

public:
  bool has_group(std::string name_) const {
    return _n2i.find(name_) != _n2i.end();
  }

  auto &add_group(std::string name_) {
    size_t index = _i2d.size();
    auto [it, inserted] = _n2i.insert({name_, index});
    if (inserted) {
      _i2d.resize(index + 1);
    }
    return _i2d[it->second];
  }

private:
  uniforms_t<Scalar> _gs;
  uniforms_t<Scalar, N> _gv;
  uniforms_t<Scalar, N, N> _gm;

  std::unordered_map<std::string, size_t> _n2i;
  std::vector<group<Scalar, N>> _i2d;
};

} // namespace prtcl::data

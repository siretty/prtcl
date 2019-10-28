#pragma once

#include <prtcl/data/tensors.hpp>

#include <string>
#include <unordered_map>
#include <utility>

namespace prtcl::data {

template <typename Scalar, typename Shape> class uniforms {
public:
  using data_type = ::prtcl::data::tensors<Scalar, Shape>;

public:
  bool has(std::string name_) const { return _n2i.find(name_) != _n2i.end(); }

  void add(std::string name_) {
    size_t index = _i2d.size();
    auto [it, inserted] = _n2i.insert({name_, index});
    if (inserted) {
      ::prtcl::detail::resize_access::resize(_i2d, index + 1);
    }
  }

private:
  std::unordered_map<std::string, size_t> _n2i;
  data_type _i2d;
};

template <typename T, size_t... Ns>
using uniforms_t = uniforms<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

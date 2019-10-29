#pragma once

#include <prtcl/data/tensors.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace prtcl::detail {

struct uniforms_access {
  template <typename Self, typename... Args> static auto &i2d(Self &&self_) {
    return std::forward<Self>(self_)._i2d;
  }
};

} // namespace prtcl::detail

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

  std::optional<size_t> get_index(std::string name_) {
    if (auto it = _n2i.find(name_); it != _n2i.end())
      return it->second;
    else
      return std::nullopt;
  }

private:
  std::unordered_map<std::string, size_t> _n2i;
  data_type _i2d;

  friend ::prtcl::detail::uniforms_access;
};

template <typename T, size_t... Ns>
using uniforms_t = uniforms<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

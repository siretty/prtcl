#pragma once

#include <prtcl/data/tensors.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/range/adaptor/transformed.hpp>

namespace prtcl::detail {

struct uniforms_access {
  template <typename Self, typename... Args> static auto &i2d(Self &&self_) {
    return *(std::forward<Self>(self_)._i2d);
  }

  template <typename Self, typename... Args> static auto &n2i(Self &&self_) {
    return std::forward<Self>(self_)._n2i;
  }
};

} // namespace prtcl::detail

namespace prtcl::data {

template <typename Scalar, typename Shape> class uniforms {
public:
  using data_type = ::prtcl::data::tensors<Scalar, Shape>;

public:
  bool has(std::string name_) const { return _n2i.find(name_) != _n2i.end(); }

  decltype(auto) add(std::string name_) {
    size_t index = _i2d->size();
    auto [it, inserted] = _n2i.insert({name_, index});
    if (inserted) {
      ::prtcl::detail::resize_access::resize(*_i2d, index + 1);
    }
    return (*_i2d)[it->second];
  }

  std::optional<size_t> get_index(std::string name_) const {
    if (auto it = _n2i.find(name_); it != _n2i.end())
      return it->second;
    else
      return std::nullopt;
  }

public:
  auto names() const {
    return boost::adaptors::transform(
        _n2i, [](auto &&kv) { return std::forward<decltype(kv)>(kv).first; });
  }

public:
  decltype(auto) operator[](size_t field_) const { return (*_i2d)[field_]; }

  decltype(auto) operator[](std::string name_) const {
    return (*this)[get_index(name_).value()];
  }

private:
  std::unordered_map<std::string, size_t> _n2i;
  std::unique_ptr<data_type> _i2d = std::make_unique<data_type>();

  friend ::prtcl::detail::uniforms_access;
};

template <typename T, size_t... Ns>
using uniforms_t = uniforms<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

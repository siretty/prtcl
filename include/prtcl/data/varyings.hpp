#pragma once

#include <prtcl/data/tensors.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/range/adaptor/transformed.hpp>

namespace prtcl::detail {

struct varyings_access {
  template <typename Self, typename... Args> static auto &i2d(Self &&self_) {
    return std::forward<Self>(self_)._i2d;
  }

  template <typename Self, typename... Args> static auto &n2i(Self &&self_) {
    return std::forward<Self>(self_)._n2i;
  }
};

} // namespace prtcl::detail

namespace prtcl::data {

template <typename Scalar, typename Shape> class varyings {
public:
  using data_type = ::prtcl::data::tensors<Scalar, Shape>;

public:
  size_t size() const { return _size; }

private:
  void resize(size_t new_size_) {
    for (auto &d : _i2d)
      ::prtcl::detail::resize_access::resize(*d, new_size_);

    _size = new_size_;
  }

  void clear() {
    for (auto &d : _i2d)
      ::prtcl::detail::resize_access::clear(*d);

    _size = 0;
  }

  friend ::prtcl::detail::resize_access;

public:
  bool has(std::string name_) const { return _n2i.find(name_) != _n2i.end(); }

  auto &add(std::string name_) {
    size_t index = _i2d.size();
    auto [it, inserted] = _n2i.insert({name_, index});
    if (inserted) {
      _i2d.resize(index + 1);
      _i2d[it->second] = std::make_unique<data_type>();
      ::prtcl::detail::resize_access::resize(*_i2d[it->second], _size);
    }
    return *_i2d[it->second];
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
  auto &operator[](size_t field_) const { return *_i2d[field_]; }

  auto &operator[](std::string name_) const {
    return (*this)[get_index(name_).value()];
  }

private:
  size_t _size = 0;
  std::unordered_map<std::string, size_t> _n2i;
  std::vector<std::unique_ptr<data_type>> _i2d;

  friend ::prtcl::detail::varyings_access;
};

template <typename T, size_t... Ns>
using varyings_t = varyings<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data

#pragma once

#include <prtcl/data/openmp/tensors.hpp>
#include <prtcl/data/varyings.hpp>

#include <utility>
#include <vector>

namespace prtcl::data::openmp {

template <typename Scalar, typename Shape> class varyings {
public:
  using data_type = ::prtcl::data::openmp::tensors<Scalar, Shape>;

public:
  varyings() = delete;

  varyings(varyings const &) = default;
  varyings &operator=(varyings const &) = default;

  varyings(varyings &&) = default;
  varyings &operator=(varyings &&) = default;

  explicit varyings(::prtcl::data::varyings<Scalar, Shape> &from_)
      : _size{from_.size()} {
    auto &from_i2d = ::prtcl::detail::varyings_access::i2d(from_);
    _i2d.reserve(from_i2d.size());
    for (size_t field = 0; field < from_i2d.size(); ++field)
      _i2d.emplace_back(*from_i2d[field]);
  }

public:
  size_t field_count() const { return _i2d.size(); }

public:
  size_t size() const { return _size; }

public:
  auto const &operator[](size_t field_) const { return _i2d[field_]; }

private:
  size_t _size = 0;
  std::vector<data_type> _i2d;
};

template <typename Scalar, typename Shape>
varyings(::prtcl::data::varyings<Scalar, Shape> &)->varyings<Scalar, Shape>;

template <typename T, size_t... Ns>
using varyings_t = varyings<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data::openmp

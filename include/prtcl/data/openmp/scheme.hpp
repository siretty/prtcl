#pragma once

#include <prtcl/data/openmp/group.hpp>
#include <prtcl/data/openmp/uniforms.hpp>
#include <prtcl/data/scheme.hpp>

#include <vector>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class scheme {
public:
  scheme() = delete;

  scheme(scheme const &) = default;
  scheme &operator=(scheme const &) = default;

  scheme(scheme &&) = default;
  scheme &operator=(scheme &&) = default;

  explicit scheme(::prtcl::data::scheme<Scalar, N> &from_)
      : _fields{_make_field_map(from_)} {
    auto &from_i2d = ::prtcl::detail::scheme_access::i2d(from_);
    _i2d.reserve(from_i2d.size());
    for (size_t group = 0; group < from_i2d.size(); ++group)
      _i2d.emplace_back(*from_i2d[group]);
  }

public:
  // get(tag::type::...) [const] -> ... {{{

  template <typename TT, typename = std::enable_if_t<tag::is_type_v<TT>>>
  auto &get(TT &&) {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <typename TT, typename = std::enable_if_t<tag::is_type_v<TT>>>
  auto &get(TT &&) const {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  // TODO: or is a "consistent" interface between group and scheme better?
  //       ie. include tag::kind::group as an unused first argument?

  // }}}

public:
  size_t get_group_count() const { return _i2d.size(); }

  auto const &get_group(size_t group_) const { return _i2d[group_]; }

private:
  // {{{ _make_field_map(data::scheme &) -> ...

  static auto _make_field_map(::prtcl::data::scheme<Scalar, N> &scheme_) {
    using boost::hana::type_c, boost::hana::make_pair;
    return boost::hana::make_map(
        make_pair(type_c<tag::type::scalar>,
                  uniforms_t<Scalar>{scheme_.get(tag::type::scalar{})}),
        make_pair(type_c<tag::type::vector>,
                  uniforms_t<Scalar, N>{scheme_.get(tag::type::vector{})}),
        make_pair(type_c<tag::type::matrix>,
                  uniforms_t<Scalar, N, N>{scheme_.get(tag::type::matrix{})}));
  }

  // }}}

  using field_map_type = decltype(
      _make_field_map(std::declval<::prtcl::data::scheme<Scalar, N> &>()));

private:
  field_map_type _fields;

  std::vector<group<Scalar, N>> _i2d;
};

template <typename Scalar, size_t N>
scheme(::prtcl::data::scheme<Scalar, N> &)->scheme<Scalar, N>;

// make compatible with ..._uniform_grid (this is a bit hacky)

template <typename Scalar, size_t N>
auto get_group_count(scheme<Scalar, N> const &scheme_) {
  return scheme_.get_group_count();
}

template <typename Scalar, size_t N>
auto const &get_group_ref(scheme<Scalar, N> const &scheme_, size_t group_) {
  return scheme_.get_group(group_);
}

} // namespace prtcl::data::openmp

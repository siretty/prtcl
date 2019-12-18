#pragma once

#include <boost/range/range_fwd.hpp>
#include <prtcl/data/group.hpp>
#include <prtcl/data/uniforms.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/scheme_requirements.hpp>
#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/bimap.hpp>
#include <boost/hana.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

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
  // get(tag::type::...) [const] -> ... {{{

  template <typename TT, typename = std::enable_if_t<tag::is_type_v<TT>>>
  auto &get(TT &&) {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <typename TT, typename = std::enable_if_t<tag::is_type_v<TT>>>
  auto &get(TT &&) const {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <typename TT>
  decltype(auto)
  get(::prtcl::expr::field<tag::kind::global, TT> const &field_) {
    return get(field_.type_tag)[field_.value];
  }

  template <typename TT>
  decltype(auto)
  get(::prtcl::expr::field<tag::kind::global, TT> const &field_) const {
    return get(field_.type_tag)[field_.value];
  }

  // TODO: or is a "consistent" interface between group and scheme better?
  //       ie. include tag::kind::group as an unused first argument?

  // }}}

public:
  // add(expr::field<...> const &) -> ... {{{

  template <typename TT>
  decltype(auto)
  add(::prtcl::expr::field<tag::kind::global, TT> const &field_) {
    return get(field_.type_tag).add(field_.value);
  }

  // }}}

public:
  bool has_group(std::string name_) const {
    return _n2i.left.find(name_) != _n2i.left.end();
  }

  auto &add_group(std::string name_) {
    size_t index = _i2d.size();
    auto [it, inserted] = _n2i.left.insert({name_, index});
    if (inserted) {
      _i2d.resize(index + 1);
      _i2d[index] = std::make_unique<group<Scalar, N>>();
    }
    return *_i2d[it->second];
  }

  size_t get_group_count() const { return _i2d.size(); }

  std::optional<size_t> get_group_index(std::string name_) const {
    if (auto it = _n2i.left.find(name_); it != _n2i.left.end())
      return it->second;
    else
      return std::nullopt;
  }

  std::optional<std::string> get_group_name(size_t index_) const {
    if (auto it = _n2i.right.find(index_); it != _n2i.right.end())
      return it->second;
    else
      return std::nullopt;
  }

  auto &get_group(size_t index_) { return *_i2d[index_]; }
  auto &get_group(size_t index_) const { return *_i2d[index_]; }

  auto &get_group(std::string name_) const {
    return get_group(get_group_index(name_));
  }

  auto groups() const {
    return boost::adaptors::transform(
        _i2d, [](auto &ptr_) -> auto & {
          if (!ptr_)
            throw "invalid group";
          return *ptr_;
        });
  }

  auto enumerate_groups() const {
    return boost::irange<size_t>(0, _i2d.size()) |
           boost::adaptors::transformed([this](auto i_) {
             return std::tuple<size_t, std::string, group<Scalar, N> const &>{
                 i_, this->get_group_name(i_).value(), this->get_group(i_)};
           });
  }

  // fullfill_requirements(...) {{{

  void fullfill_requirements(expr::scheme_requirements const &reqs_) {
    // TODO: potentially rename globals into scheme_fields (maybe in conjunction
    //       with the consistent interface for scheme.get(...))
    for (auto &&fvar : reqs_.globals)
      std::visit([this](auto const &f) { this->get(f.type_tag).add(f.value); },
                 std::forward<decltype(fvar)>(fvar));
    // TODO: potentially merge uniforms and varyings into group_fields
    for (size_t gi = 0; gi < get_group_count(); ++gi) {
      for (auto &&[pred, fvar] : reqs_.uniforms) {
        auto &g = get_group(gi);
        if (std::forward<decltype(pred)>(pred)(g))
          std::visit(
              [&g](auto const &f) {
                g.get(f.kind_tag, f.type_tag).add(f.value);
              },
              std::forward<decltype(fvar)>(fvar));
      }
      for (auto &&[pred, fvar] : reqs_.varyings) {
        auto &g = get_group(gi);
        if (std::forward<decltype(pred)>(pred)(g))
          std::visit(
              [&g](auto const &f) {
                g.get(f.kind_tag, f.type_tag).add(f.value);
              },
              std::forward<decltype(fvar)>(fvar));
      }
    }
  }

  // }}}

private:
  // _make_field_map() -> ... {{{
  static auto _make_field_map() {
    using boost::hana::type_c, boost::hana::make_pair;
    return boost::hana::make_map(
        make_pair(type_c<tag::type::scalar>, uniforms_t<Scalar>{}),
        make_pair(type_c<tag::type::vector>, uniforms_t<Scalar, N>{}),
        make_pair(type_c<tag::type::matrix>, uniforms_t<Scalar, N, N>{}));
  }
  // }}}

  using field_map_type = decltype(_make_field_map());

private:
  field_map_type _fields;

  boost::bimap<std::string, size_t> _n2i;
  std::vector<std::unique_ptr<group<Scalar, N>>> _i2d;

  friend ::prtcl::detail::scheme_access;
};

} // namespace prtcl::data

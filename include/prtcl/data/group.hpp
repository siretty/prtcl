#pragma once

#include <prtcl/data/group_base.hpp>
#include <prtcl/data/uniforms.hpp>
#include <prtcl/data/varyings.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <type_traits>
#include <unordered_set>

#include <boost/hana.hpp>

namespace prtcl::data {

template <typename Scalar, size_t N> class group : public group_base {
public:
  size_t size() const { return _size; }

  void resize(size_t new_size_) {
    get(tag::kind::varying{}, tag::type::scalar{}).resize(new_size_);
    get(tag::kind::varying{}, tag::type::vector{}).resize(new_size_);
    get(tag::kind::varying{}, tag::type::matrix{}).resize(new_size_);

    _size = new_size_;
  }

public:
  // get(tag::kind::..., tag::type::...) [const] -> ... {{{

  template <
      typename KT, typename TT,
      typename = std::enable_if_t<tag::is_kind_v<KT> and tag::is_type_v<TT>>>
  auto &get(KT &&, TT &&) {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<KT>>]
                  [boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <
      typename KT, typename TT,
      typename = std::enable_if_t<tag::is_kind_v<KT> and tag::is_type_v<TT>>>
  auto &get(KT &&, TT &&) const {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<KT>>]
                  [boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <typename KT, typename TT>
  decltype(auto) get(::prtcl::expr::field<KT, TT> const &field_) {
    return get(field_.kind_tag, field_.type_tag)[field_.value];
  }

  template <typename KT, typename TT>
  decltype(auto) get(::prtcl::expr::field<KT, TT> const &field_) const {
    return get(field_.kind_tag, field_.type_tag)[field_.value];
  }

  // }}}

public:
  // add(expr::field<...> const &) -> ... {{{

  template <typename KT, typename TT>
  decltype(auto) add(::prtcl::expr::field<KT, TT> const &field_) {
    return get(field_.kind_tag, field_.type_tag).add(field_.value);
  }

  // }}}

public:
  auto &set_type(std::string type_) {
    _type = type_;
    return *this;
  }

  std::string get_type() const override { return _type; }

private:
  // {{{ _make_field_map() -> ...

  static auto _make_field_map() {
    using boost::hana::make_pair;
    using boost::hana::type_c;
    constexpr auto scalar_c = type_c<tag::type::scalar>;
    constexpr auto vector_c = type_c<tag::type::vector>;
    constexpr auto matrix_c = type_c<tag::type::matrix>;
    return boost::hana::make_map(
        make_pair(type_c<tag::kind::uniform>,
                  boost::hana::make_map(
                      make_pair(scalar_c, uniforms_t<Scalar>{}),
                      make_pair(vector_c, uniforms_t<Scalar, N>{}),
                      make_pair(matrix_c, uniforms_t<Scalar, N, N>{}))),
        make_pair(type_c<tag::kind::varying>,
                  boost::hana::make_map(
                      make_pair(scalar_c, varyings_t<Scalar>{}),
                      make_pair(vector_c, varyings_t<Scalar, N>{}),
                      make_pair(matrix_c, varyings_t<Scalar, N, N>{}))));
  }

  // }}}

  using field_map_type = decltype(_make_field_map());

private:
  size_t _size = 0;
  field_map_type _fields = _make_field_map();
  std::string _type;

  // deprecate
  std::unordered_set<std::string> _flags;
};

} // namespace prtcl::data

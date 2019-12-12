#pragma once

#include <prtcl/expr/field_base.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <type_traits>
#include <variant>

#include <boost/yap/user_macros.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename KT, typename TT>
struct field : field_base<KT, TT, std::string> {
  friend std::ostream &operator<<(std::ostream &s, field const &f) {
    return s << f.value;
  }
};

// recognition type trait

template <typename> struct is_field : std::false_type {};
template <typename KT, typename TT>
struct is_field<field<KT, TT>> : std::true_type {};

template <typename T>
constexpr bool is_field_v = is_field<meta::remove_cvref_t<T>>::value;

// Boost.YAP operators

#define PRTCL_YAP_UDT_ANY_BOPS(ExprTemplate, Trait)                            \
  BOOST_YAP_USER_UDT_ANY_BINARY_OPERATOR(plus, ExprTemplate, Trait)            \
  BOOST_YAP_USER_UDT_ANY_BINARY_OPERATOR(minus, ExprTemplate, Trait)           \
  BOOST_YAP_USER_UDT_ANY_BINARY_OPERATOR(multiplies, ExprTemplate, Trait)      \
  BOOST_YAP_USER_UDT_ANY_BINARY_OPERATOR(divides, ExprTemplate, Trait)

PRTCL_YAP_UDT_ANY_BOPS(::boost::yap::expression, ::prtcl::expr::is_field)

#undef PRTCL_YAP_UDT_ANY_BOPS

} // namespace prtcl::expr

// user defined literals

#define PRTCL_DEFINE_FIELD_ALIASES(Name, KindTag, TypeTag)                     \
  namespace prtcl::expr {                                                      \
  using Name##_field = ::prtcl::expr::field<::prtcl::tag::kind::KindTag,       \
                                            ::prtcl::tag::type::TypeTag>;      \
  } /* namespace prtcl::expr */                                                \
  namespace prtcl::expr_literals {                                             \
  inline auto operator""_##Name##f(char const *ptr, size_t len) {              \
    return ::prtcl::expr::field<::prtcl::tag::kind::KindTag,                   \
                                ::prtcl::tag::type::TypeTag>{                  \
        {std::string{ptr, len}}};                                              \
  }                                                                            \
  inline auto operator""_##Name(char const *ptr, size_t len) {                 \
    return ::boost::yap::make_terminal(                                        \
        ::prtcl::expr::field<::prtcl::tag::kind::KindTag,                      \
                             ::prtcl::tag::type::TypeTag>{                     \
            {std::string{ptr, len}}});                                         \
  }                                                                            \
  } /* namespace prtcl::expr_literals */

PRTCL_DEFINE_FIELD_ALIASES(gs, global, scalar)
PRTCL_DEFINE_FIELD_ALIASES(gv, global, vector)
PRTCL_DEFINE_FIELD_ALIASES(gm, global, matrix)

PRTCL_DEFINE_FIELD_ALIASES(us, uniform, scalar)
PRTCL_DEFINE_FIELD_ALIASES(uv, uniform, vector)
PRTCL_DEFINE_FIELD_ALIASES(um, uniform, matrix)

PRTCL_DEFINE_FIELD_ALIASES(vs, varying, scalar)
PRTCL_DEFINE_FIELD_ALIASES(vv, varying, vector)
PRTCL_DEFINE_FIELD_ALIASES(vm, varying, matrix)

#undef PRTCL_DEFINE_FIELD_ALIASES

namespace prtcl::expr {

using any_global_field = std::variant<gs_field, gv_field, gm_field>;
using any_uniform_field = std::variant<us_field, uv_field, um_field>;
using any_varying_field = std::variant<vs_field, vv_field, vm_field>;

} // namespace prtcl::expr

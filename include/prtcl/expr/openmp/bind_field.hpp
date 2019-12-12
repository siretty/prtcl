#pragma once

#include <boost/yap/expression.hpp>
#include <prtcl/data/openmp/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/openmp/prepared_loop.hpp>
#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/tag/group.hpp>
#include <prtcl/tag/kind.hpp>

#include <boost/hana.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr::openmp {

template <typename Scalar, size_t N>
class bind_global_field_xform : private xform_helper {
public:
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
  template <typename TT>
  decltype(auto) operator()(term<field<tag::kind::global, TT>> term_) const {
    auto const &field = term_.value();
    auto &storage = _scheme.get(field.type_tag);
    size_t field_index = storage.get_index(field.value);
    return boost::yap::make_terminal(storage[field_index]);
  }

public:
  explicit bind_global_field_xform(scheme_type &scheme_) : _scheme{scheme_} {}

private:
  scheme_type &_scheme;
};

template <typename Scalar, size_t N, typename GT>
class bind_uniform_field_xform : private xform_helper {
  static_assert(tag::is_group_v<GT>);

public:
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
  template <typename TT>
  decltype(auto)
  operator()(subs<term<field<tag::kind::uniform, TT>>, term<GT>> subs_) const {
    auto &group = _scheme.get_group(_group_index);
    auto const &field = subs_.left().value();
    auto &field_storage = group.get(field.kind_tag, field.type_tag);
    size_t field_index = field_storage.get_index(field.value);
    return boost::yap::make_terminal(field_storage[field_index]);
  }

public:
  bind_uniform_field_xform(scheme_type &scheme_, size_t group_index_)
      : _scheme{scheme_}, _group_index{group_index_} {}

private:
  scheme_type &_scheme;
  size_t _group_index;
};

template <typename Scalar, size_t N, typename GT>
class bind_varying_field_xform : private xform_helper {
  static_assert(tag::is_group_v<GT>);

public:
  using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
  template <typename TT>
  decltype(auto)
  operator()(subs<term<field<tag::kind::varying, TT>>, term<GT>> subs_) const {
    auto &group = _scheme.get_group(_group_index);
    auto const &field = subs_.left().value();
    auto &field_storage = group.get(field.kind_tag, field.type_tag);
    size_t field_index = field_storage.get_index(field.value);
    return boost::yap::make_expression<expr_kind::subscript>(
        boost::yap::make_terminal(field_storage[field_index]), subs_.right());
  }

public:
  bind_varying_field_xform(scheme_type &scheme_, size_t group_index_)
      : _scheme{scheme_}, _group_index{group_index_} {}

private:
  scheme_type &_scheme;
  size_t _group_index;
};

/*
template <typename E> struct bound_eq { E expression; };

template <typename Scalar, size_t N>
class bind_section_eq : private xform_helper {
public:
using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
//

public:
bind_section_eq(scheme_type &scheme_) : _scheme{scheme_} {}

private:
scheme_type &_scheme;
};

template <typename Scalar, size_t N>
struct bind_particle_eq : private xform_helper {
public:
using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
//

public:
bind_particle_eq(scheme_type &scheme_) : _scheme{scheme_} {}

private:
scheme_type &_scheme;
};

template <typename Scalar, size_t N>
struct bind_neighbour_eq : private xform_helper {
public:
using scheme_type = ::prtcl::data::openmp::scheme<Scalar, N>;

public:
//

public:
bind_neighbour_eq(scheme_type &scheme_) : _scheme{scheme_} {}

private:
scheme_type &_scheme;
};

// ----

template <typename E> auto make_eq(E &&e) {
return eq<meta::remove_cvref_t<E>>{e};
}

template <typename F> class bind_eq_xform_base {
public:
template <typename U>
explicit bind_eq_xform_base(U &&callable_)
    : _callable{std::forward<U>(callable_)} {}

protected:
template <typename E> decltype(auto) call(E &&e) {
  return _callable(std::forward<E>(e));
}

template <typename E> decltype(auto) call(E &&e) const {
  return _callable(std::forward<E>(e));
}

private:
F _callable;
};

template <template <typename> typename T, typename F>
auto make_bind_eq_xform(F &&callable_) {
return T<meta::remove_cvref_t<F>>{std::forward<F>(callable_)};
}

template <template <typename> typename T> auto make_bind_eq_xform() {
return make_bind_eq_xform<T>(
    [](auto &&e) -> auto { return std::forward<decltype(e)>(e); });
}

template <typename F>
struct geq_xform : private xform_helper, public eq_xform_base<F> {
template <expr_kind K, typename TT, typename RHS,
          typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
decltype(auto)
operator()(expr<K, term<field<tag::kind::global, TT>>, RHS> expr) const {
  return this->call(expr);
}

using eq_xform_base<F>::eq_xform_base;
};

template <typename F>
struct ueq_xform : private xform_helper, public eq_xform_base<F> {
template <expr_kind K, typename TT, typename RHS,
          typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
decltype(auto) operator()(
    expr<K,
         subs<term<field<tag::kind::uniform, TT>>, term<tag::group::active>>,
         RHS>
        expr) const {
  return this->call(expr);
}

using eq_xform_base<F>::eq_xform_base;
};

template <typename F>
struct veq_xform : private xform_helper, public eq_xform_base<F> {
template <expr_kind K, typename TT, typename RHS,
          typename = std::enable_if_t<is_assign_v<K> or is_opassign_v<K>>>
decltype(auto) operator()(
    expr<K,
         subs<term<field<tag::kind::varying, TT>>, term<tag::group::active>>,
         RHS>
        expr) const {
  return this->call(expr);
}

using eq_xform_base<F>::eq_xform_base;
};

template <typename Expr> auto _geq(Expr &&e0) {
auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                group_tag_terminal_value_xform{});
return boost::yap::transform_strict(
    e1, make_eq_xform<geq_xform>([](auto) { return true; }),
    [](auto) { return false; });
}

template <typename Expr> auto _ueq(Expr &&e0) {
auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                group_tag_terminal_value_xform{});
return boost::yap::transform_strict(
    e1, make_eq_xform<ueq_xform>([](auto) { return true; }),
    [](auto) { return false; });
}

template <typename Expr> auto _veq(Expr &&e0) {
auto e1 = boost::yap::transform(std::forward<Expr>(e0),
                                group_tag_terminal_value_xform{});
return boost::yap::transform_strict(
    e1, make_eq_xform<veq_xform>([](auto) { return true; }),
    [](auto) { return false; });
}
*/

} // namespace prtcl::expr::openmp

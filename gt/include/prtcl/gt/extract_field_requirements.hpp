#pragma once

#include <prtcl/gt/eq.hpp>
#include <prtcl/gt/field.hpp>
#include <prtcl/gt/index_kind.hpp>
#include <prtcl/gt/select_group_type.hpp>

#include <prtcl/gt/field_requirements.hpp>
#include <prtcl/gt/xform_helper.hpp>

#include <optional>

#include <boost/hana.hpp>

namespace prtcl::gt {

class extract_field_requirements_xform : xform_helper {
  // ... {{{
public:
  /// Record a solitary field terminal.
  auto operator()(term<field> term_) {
    auto f = term_.value();
    switch (f.kind()) {
    case field_kind::global: {
      _reqs.add_requirement(f);
    } break;
    default:
      throw "invalid field kind";
    }
    return term_;
  }

  /// Record a subscripted field terminal.
  auto operator()(subs<term<field>, term<index_kind>> subs_) {
    auto f = subs_.left().value();
    auto i = subs_.right().value();
    switch (f.kind()) {
    case field_kind::uniform:
    case field_kind::varying: {
      switch (i) {
      case index_kind::particle: {
        _reqs.add_requirement(_cur_p.value(), f);
      } break;
      case index_kind::neighbour: {
        _reqs.add_requirement(_cur_n.value(), f);
      } break;
      default:
        throw "invalid index kind";
      }
    } break;
    default:
      throw "invalid field kind";
    }
    return subs_;
  }

  /// Record fields in equations.
  template <typename Expr_> auto operator()(term<eq<Expr_>> term_) {
    auto e = term_.value();

    // extract lhs field
    switch (e.field.kind()) {
    case field_kind::global: {
      _reqs.add_requirement(e.field);
    } break;
    case field_kind::uniform:
    case field_kind::varying: {
      _reqs.add_requirement(_cur_p.value(), e.field);
    } break;
    default:
      throw "invalid field kind";
    }
    // TODO: add error checking?

    // transform the expression
    boost::yap::transform(term_.value().expression, *this);

    return term_;
  }

  /// Fill in the appropriate group selector.
  template <typename... Exprs>
  auto operator()(call<term<select_group_type>, Exprs...> call_) {
    // extract the select object
    using namespace boost::hana::literals;
    auto &selector = call_.elements[0_c].value();

    // fill in the current group types
    if (!_cur_p && !_cur_n)
      _cur_p = selector.group_type();
    else if (_cur_p && !_cur_n)
      _cur_n = selector.group_type();
    else
      throw "invalid selector";

    // transform the statements
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(call_.elements)>(
            call_.elements),
        [this](auto &&e_) {
          boost::yap::transform(std::forward<decltype(e_)>(e_), *this);
        });

    // clear the current group types
    if (_cur_p && _cur_n)
      _cur_n.reset();
    else if (_cur_p && !_cur_n)
      _cur_p.reset();
    else
      throw "invalid selector";

    return call_;
  }

public:
  extract_field_requirements_xform(field_requirements &reqs_) : _reqs{reqs_} {}

private:
  field_requirements &_reqs;
  std::optional<std::string> _cur_p, _cur_n;
  // }}}
};

template <typename Expr>
inline void
extract_field_requirements(field_requirements &reqs_, Expr &&expr_) {
  boost::yap::transform(
      std::forward<Expr>(expr_), extract_field_requirements_xform{reqs_});
}

} // namespace prtcl::gt

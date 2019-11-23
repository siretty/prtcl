#pragma once

#include "prtcl/meta/is_any_of.hpp"
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/tags.hpp>

#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

/// field<varying, T, _, V>[group<G>] -> field<varying, T, G, V>[group<G>]
/// field<uniform, T, _, V>[group<G>] -> field<uniform, T, G, V>
/// field<global, *, *, *>[group<*>] -> error
struct field_subscript_transform {
  template <typename KT, typename TT, typename V, typename GT>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::subscript>,
                  field<KT, TT, tag::unspecified, V> const &field_,
                  group<GT> group_) const {
    static_assert(meta::is_any_of_v<KT, tag::varying, tag::uniform>);
    return term(field_.with_group(GT{}))[term(group_)];
  }

  template <typename KT, typename TT, typename V, typename GT>
  auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::subscript>,
                  field<KT, TT, tag::unspecified, V> &&field_,
                  group<GT> group_) const {
    static_assert(meta::is_any_of_v<KT, tag::varying, tag::uniform>);
    return term(field_.with_group(GT{}))[term(group_)];
  }
};

} // namespace prtcl::expr

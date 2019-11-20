#pragma once

#include "../tags.hpp"
#include "field.hpp"
#include "group.hpp"

#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct field_subscript_transform {
  template <typename KT, typename TT, typename V, typename GT>
  field_term<KT, TT, GT, V>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::subscript>,
             field<KT, TT, tag::unspecified, V> const &field_,
             group<GT>) const {
    return {{field_.value}};
  }

  template <typename KT, typename TT, typename V, typename GT>
  field_term<KT, TT, GT, V>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::subscript>,
             field<KT, TT, tag::unspecified, V> &&field_, group<GT>) const {
    return {{std::move(field_.value)}};
  }
};

} // namespace prtcl::expr

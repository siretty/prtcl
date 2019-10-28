#pragma once

#include "../meta/remove_cvref.hpp"
#include "field.hpp"

#include <functional>
#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename F> struct field_value_transform {
  template <typename KT, typename TT, typename GT, typename AT, typename V>
  field_term<KT, TT, GT, AT,
             meta::remove_cvref_t<std::invoke_result_t<F, KT, TT, GT, AT, V>>>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
             field<KT, TT, GT, AT, V> const &field_) const {
    return {{std::invoke(f, KT{}, TT{}, GT{}, AT{}, field_.value)}};
  }

  template <typename KT, typename TT, typename GT, typename AT, typename V>
  field_term<KT, TT, GT, AT,
             meta::remove_cvref_t<std::invoke_result_t<F, KT, TT, GT, AT, V>>>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
             field<KT, TT, GT, AT, V> &&field_) const {
    return {{std::invoke(f, KT{}, TT{}, GT{}, AT{}, std::move(field_.value))}};
  }

  F f;
};

template <typename F>
field_value_transform<meta::remove_cvref_t<F>>
make_field_value_transform(F &&f) {
  return {std::forward<F>(f)};
}

} // namespace prtcl::expr

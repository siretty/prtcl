#pragma once

#include "../meta/remove_cvref.hpp"
#include "field.hpp"

#include <functional>
#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename F> struct field_value_transform {
  template <typename KT, typename TT, typename GT, typename V>
  field_term<KT, TT, GT, remove_cvref_t<std::invoke_result_t<F, V>>>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
             field<KT, TT, GT, V> const &field_) const {
    return {{std::invoke(f, field_.value)}};
  }

  template <typename KT, typename TT, typename GT, typename V>
  field_term<KT, TT, GT, remove_cvref_t<std::invoke_result_t<F, V>>>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
             field<KT, TT, GT, V> &&field_) const {
    return {{std::invoke(f, std::move(field_.value))}};
  }

  F f;
};

template <typename F>
field_value_transform<remove_cvref_t<F>> make_field_value_transform(F &&f) {
  return {std::forward<F>(f)};
}

} // namespace prtcl::expr

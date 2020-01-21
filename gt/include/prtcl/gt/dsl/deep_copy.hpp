#pragma once

#include <boost/yap/yap.hpp>

namespace prtcl::gt::dsl {

// ============================================================
// Helper Transforms
// ============================================================

// ------------------------------------------------------------
// Remove Expresion-Ref. Transform
// ------------------------------------------------------------

struct remove_expr_ref_xform {
private:
  template <typename Expr_>
  using expr_ref_t =
      boost::yap::expression<boost::yap::expr_kind::expr_ref, Expr_>;

public:
  template <typename Expr_>
  decltype(auto) operator()(expr_ref_t<Expr_> ref_) const {
    return boost::yap::transform(boost::yap::deref(ref_), *this);
  }
};

template <typename Expr_> auto deep_copy(Expr_ &&expr_) {
  return boost::yap::transform(
      std::forward<Expr_>(expr_), remove_expr_ref_xform{});
}

} // namespace prtcl::gt::dsl

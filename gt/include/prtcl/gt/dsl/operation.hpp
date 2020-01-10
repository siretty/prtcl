#pragma once

#include "common.hpp"

#include <string>

namespace prtcl::gt::dsl {

struct operation {
  std::string name;
};

using operation_expr = term_type<operation>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename LHS_, typename RHS_> auto dot(LHS_ &&lhs_, RHS_ &&rhs_) {
  return operation_expr{{"dot"}}(
      std::forward<LHS_>(lhs_), std::forward<RHS_>(rhs_));
}

template <typename Expr_> auto norm(Expr_ &&expr_) {
  return operation_expr{{"norm"}}(std::forward<Expr_>(expr_));
}

template <typename Expr_> auto norm_squared(Expr_ &&expr_) {
  return operation_expr{{"norm_squared"}}(std::forward<Expr_>(expr_));
}

template <typename Expr_> auto normalized(Expr_ &&expr_) {
  return operation_expr{{"normalized"}}(std::forward<Expr_>(expr_));
}

template <typename... Exprs_> auto max(Exprs_ &&... exprs_) {
  return operation_expr{{"max"}}(std::forward<Exprs_>(exprs_)...);
}

template <typename... Exprs_> auto min(Exprs_ &&... exprs_) {
  return operation_expr{{"min"}}(std::forward<Exprs_>(exprs_)...);
}

// =====
// Kernel
// =====

template <typename Expr_> auto kernel(Expr_ &&expr_) {
  return operation_expr{{"kernel"}}(std::forward<Expr_>(expr_));
}

template <typename DeltaExpr_, typename ScaleExpr_>
auto kernel_h(DeltaExpr_ &&delta_expr_, ScaleExpr_ &&scale_expr_) {
  return operation_expr{{"kernel_h"}}(
      std::forward<DeltaExpr_>(delta_expr_),
      std::forward<ScaleExpr_>(scale_expr_));
}

// =====
// Kernel Gradient
// =====

template <typename Expr_> auto kernel_gradient(Expr_ &&expr_) {
  return operation_expr{{"kernel_gradient"}}(std::forward<Expr_>(expr_));
}

template <typename DeltaExpr_, typename ScaleExpr_>
auto kernel_gradient_h(DeltaExpr_ &&delta_expr_, ScaleExpr_ &&scale_expr_) {
  return operation_expr{{"kernel_gradient_h"}}(
      std::forward<DeltaExpr_>(delta_expr_),
      std::forward<ScaleExpr_>(scale_expr_));
}

} // namespace prtcl::gt::dsl::language

namespace prtcl::gt::dsl::kernel_shorthand {

template <typename... Exprs_> auto W(Exprs_ &&... exprs_) {
  return ::prtcl::gt::dsl::language::kernel(std::forward<Exprs_>(exprs_)...);
}

template <typename... Exprs_> auto W_h(Exprs_ &&... exprs_) {
  return ::prtcl::gt::dsl::language::kernel_h(std::forward<Exprs_>(exprs_)...);
}

template <typename... Exprs_> auto dW(Exprs_ &&... exprs_) {
  return ::prtcl::gt::dsl::language::kernel_gradient(
      std::forward<Exprs_>(exprs_)...);
}

template <typename... Exprs_> auto dW_h(Exprs_ &&... exprs_) {
  return ::prtcl::gt::dsl::language::kernel_gradient_h(
      std::forward<Exprs_>(exprs_)...);
}

} // namespace prtcl::gt::dsl::kernel_shorthand

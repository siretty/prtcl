#pragma once

#include "common.hpp"

namespace prtcl::gt::dsl {

template <typename Expr_> struct equation { Expr_ expression; };

template <typename Expr_> using equation_expr = term_type<equation<Expr_>>;

} // namespace prtcl::gt::dsl

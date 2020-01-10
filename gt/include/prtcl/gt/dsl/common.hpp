#pragma once

#include <utility>

#include <boost/yap/expression.hpp>

#include <boost/hana/slice.hpp>
#include <boost/hana/tuple.hpp>

namespace prtcl::gt::dsl {

using expr_kind = boost::yap::expr_kind;

template <expr_kind Kind_, typename... Types_>
using expr_type = boost::yap::expression<Kind_, boost::hana::tuple<Types_...>>;

template <typename Type_>
using term_expr = expr_type<expr_kind::terminal, Type_>;

} // namespace prtcl::gt::dsl

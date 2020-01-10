#pragma once

#include <boost/yap/expression.hpp>

#include <boost/hana/tuple.hpp>

namespace prtcl::gt::dsl {

using expr_kind = boost::yap::expr_kind;

template <expr_kind Kind_, typename... Types_>
using expr_type = boost::yap::expression<Kind_, boost::hana::tuple<Types_...>>;

template <typename Type_>
using term_type = expr_type<expr_kind::terminal, Type_>;

template <typename Callee_, typename... Args_>
using call_type = expr_type<expr_kind::call, Callee_, Args_...>;

} // namespace prtcl::gt::dsl

#pragma once

#include "parser_define.hpp"

#include <boost/spirit/home/x3.hpp>

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
// }}}

namespace prtcl::gt::parser {

BOOST_SPIRIT_DEFINE(white_space, identifier, nd_type);

namespace math {

BOOST_SPIRIT_DEFINE(
    primary, expression, literal, constant, field_access, function_call);

BOOST_SPIRIT_DEFINE(unary_neg, unary_pos);

BOOST_SPIRIT_DEFINE(add_rhs, sub_rhs, add_term);

BOOST_SPIRIT_DEFINE(mul_rhs, div_rhs, mul_term);

} // namespace math

namespace init {

BOOST_SPIRIT_DEFINE(field, particle_selector);

} // namespace init

namespace bits {

BOOST_SPIRIT_DEFINE(
    foreach_neighbor_statement, foreach_particle_statement,
    procedure_statement);

} // namespace bits

namespace stmt {

BOOST_SPIRIT_DEFINE(let, compute, reduce);

BOOST_SPIRIT_DEFINE(foreach_neighbor, foreach_particle, procedure);

} // namespace stmt

BOOST_SPIRIT_DEFINE(statement);

BOOST_SPIRIT_DEFINE(prtcl_source_file);

} // namespace prtcl::gt::parser

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

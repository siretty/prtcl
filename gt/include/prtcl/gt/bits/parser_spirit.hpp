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

BOOST_SPIRIT_DEFINE(white_space, identifier, ndtype);

namespace n_math {

BOOST_SPIRIT_DEFINE(primary, expression)

BOOST_SPIRIT_DEFINE(literal, operation, field_access);

BOOST_SPIRIT_DEFINE(unary_neg);

BOOST_SPIRIT_DEFINE(add_rhs, sub_rhs, add_term);

BOOST_SPIRIT_DEFINE(mul_rhs, div_rhs, mul_term);

} // namespace n_math

namespace n_global {

BOOST_SPIRIT_DEFINE(field);

} // namespace n_global

BOOST_SPIRIT_DEFINE(global);

namespace n_group {

BOOST_SPIRIT_DEFINE(expression, primary);

BOOST_SPIRIT_DEFINE(select_atom);

BOOST_SPIRIT_DEFINE(logic_neg, logic_con_rhs, logic_dis_rhs, logic_term);

BOOST_SPIRIT_DEFINE(uniform_field, varying_field, field);

} // namespace n_group

BOOST_SPIRIT_DEFINE(group);

namespace n_scheme {

BOOST_SPIRIT_DEFINE(local, compute, reduce);

BOOST_SPIRIT_DEFINE(foreach_neighbor_statement, foreach_neighbor)

BOOST_SPIRIT_DEFINE(foreach_particle_statement, foreach_particle)

BOOST_SPIRIT_DEFINE(procedure_statement, procedure);

} // namespace n_scheme

BOOST_SPIRIT_DEFINE(scheme_statement, scheme);

BOOST_SPIRIT_DEFINE(prtcl_file_statement, prtcl_file);

} // namespace prtcl::gt::parser

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

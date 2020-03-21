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

namespace n_groups {

BOOST_SPIRIT_DEFINE(select_expression, select_primary);

BOOST_SPIRIT_DEFINE(select_atom);

BOOST_SPIRIT_DEFINE(
    select_logic_neg, select_logic_con_rhs, select_logic_dis_rhs,
    select_logic_term);

BOOST_SPIRIT_DEFINE(uniform_field, varying_field, field);

} // namespace n_groups

BOOST_SPIRIT_DEFINE(groups);

namespace n_scheme {

BOOST_SPIRIT_DEFINE(local, compute, reduce);

BOOST_SPIRIT_DEFINE(foreach_neighbor_statement, foreach_neighbor)

BOOST_SPIRIT_DEFINE(foreach_particle_statement, foreach_particle)

namespace n_solve {

BOOST_SPIRIT_DEFINE(statement);
BOOST_SPIRIT_DEFINE(setup, product, apply);

} // namespace n_solve

BOOST_SPIRIT_DEFINE(solve);

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

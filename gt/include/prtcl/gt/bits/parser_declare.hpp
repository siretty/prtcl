#pragma once

#include "../ast.hpp"
#include "prtcl/gt/bits/ast_define.hpp"

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

// {{{ common type aliases
namespace prtcl::gt::parser {

using std::optional;
using std::string;
using std::variant;
using std::vector;

} // namespace prtcl::gt::parser
// }}}

//  {{{ x3 aliases
namespace prtcl::gt::parser {

namespace x3 = boost::spirit::x3;

using x3::alpha, x3::alnum;
using x3::char_;
using x3::lit, x3::attr;

} // namespace prtcl::gt::parser
//  }}}

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
// }}}

// ============================================================
// Rule Declarations
// ============================================================

namespace prtcl::gt::parser {

// {{{ error_handler

struct error_handler {
  template <typename Iterator_, typename Exception_, typename Context_>
  x3::error_handler_result on_error(
      Iterator_, Iterator_, Exception_ const &error,
      Context_ const &context) const {
    auto &error_handler = x3::get<x3::error_handler_tag>(context);
    std::string message = "error: expecting " + error.which() + ":";
    error_handler(error.where(), message);
    return x3::error_handler_result::fail;
  }
};

// }}}

/// Match white space (space, tab, newline, ...) and comments (C/C++ style).
x3::rule<class white_space_class> white_space = "white_space";

/// Parse ast::dtype values.
struct dtype_symbols /* { ... } dtype */;

/// Parse ast::ndtype values.
x3::rule<class ndtype_class, ast::ndtype> ndtype = "ndtype";

/// Parse identifiers as strings.
x3::rule<class identifier_class, string> identifier = "identifier";

// {{{ mathematical expression rules

namespace n_math {

// {{{ declarations of the rule id types (..._class)

struct literal_class;
struct operation_class;
struct field_access_class;

struct unary_neg_class;

struct add_rhs_class;
struct sub_rhs_class;
struct add_term_class;

struct mul_rhs_class;
struct div_rhs_class;
struct mul_term_class;

// }}}

/// Parse a full mathematical expression.
x3::rule<class expression_class, ast::n_math::expression> const expression =
    "mathematical expression";

/// Parse primary (ie. non-arithmetic) expressions.
x3::rule<class primary_class, ast::n_math::expression> const primary =
    "mathematical primary expression";

/// Parse mathematical literals.
x3::rule<literal_class, ast::n_math::literal> const literal =
    "mathematical literal";

/// Parse function calls.
x3::rule<operation_class, ast::n_math::operation> const operation =
    "mathematical operation";

/// Parse particle field access.
x3::rule<field_access_class, ast::n_math::field_access> const field_access =
    "field access";

/// Parse a unary negation operation.
x3::rule<unary_neg_class, ast::n_math::unary_arithmetic> const unary_neg =
    "arithmetic negation";

/// Parse the right-hand-side of an addition.
x3::rule<add_rhs_class, ast::n_math::multi_arithmetic_rhs> const add_rhs =
    "arithmetic addition (rhs)";

/// Parse the right-hand-side of a subtraction.
x3::rule<sub_rhs_class, ast::n_math::multi_arithmetic_rhs> const sub_rhs =
    "arithmetic subtraction (rhs)";

/// Parse an n-ary additive operation (ie. addition or subtraction).
x3::rule<add_term_class, ast::n_math::multi_arithmetic> const add_term =
    "arithmetic additive term";

/// Parse the right-hand-side of a multiplication.
x3::rule<mul_rhs_class, ast::n_math::multi_arithmetic_rhs> const mul_rhs =
    "arithmetic multiplication (rhs)";

/// Parse the right-hand-side of a division.
x3::rule<div_rhs_class, ast::n_math::multi_arithmetic_rhs> const div_rhs =
    "arithmetic division (rhs)";

/// Parse an n-ary multiplicative operation (ie. multiplication or division).
x3::rule<mul_term_class, ast::n_math::multi_arithmetic> const mul_term =
    "arithmetic multiplicative term";

// {{{ definitions of the rule id types (..._class)

struct literal_class : x3::annotate_on_success {};
struct operation_class : x3::annotate_on_success {};
struct field_access_class : x3::annotate_on_success {};

struct unary_neg_class : x3::annotate_on_success {};

struct add_rhs_class : x3::annotate_on_success {};
struct sub_rhs_class : x3::annotate_on_success {};
struct add_term_class : x3::annotate_on_success {};

struct mul_rhs_class : x3::annotate_on_success {};
struct div_rhs_class : x3::annotate_on_success {};
struct mul_term_class : x3::annotate_on_success {};

// }}}

} // namespace n_math

// }}}

// {{{ scheme { global { ... } } rules

namespace n_global {

struct field_class;

/// Parse a global field alias definition.
x3::rule<field_class, ast::n_global::field> field = "global field alias";

struct field_class : x3::annotate_on_success {};

} // namespace n_global

struct global_class;

/// Parse a global selector.
x3::rule<global_class, ast::global> global = "global definition";

struct global_class : x3::annotate_on_success {};

// }}}

// {{{ scheme { groups ... { ... } } rules

// {{{ n_groups::... rules

namespace n_groups {

/// Parse a selector expression.
x3::rule<class select_expression_class, ast::n_groups::select_expression>
    select_expression = "groups select expression";

/// Parse a primary selector expression.
x3::rule<class select_primary_class, ast::n_groups::select_expression>
    select_primary = "primary groups select expression";

struct select_atom_kind_symbols; /* { ... } select_atom_kind; */

// {{{ declaration of the rule id types (..._class)

struct select_atom_class;
struct select_unary_logic_neg_class;
struct select_logic_con_rhs_class;
struct select_logic_dis_rhs_class;
struct select_logic_term_class;
struct uniform_field_class;
struct varying_field_class;

// }}}

/// Parse a type atom.
x3::rule<select_atom_class, ast::n_groups::select_atom> select_atom =
    "select atom";

/// Parse a unary logical negation.
x3::rule<select_unary_logic_neg_class, ast::n_groups::select_unary_logic>
    select_logic_neg = "logical negation";

/// Parse the right-hand-side of a logical conjunction.
x3::rule<select_logic_con_rhs_class, ast::n_groups::select_multi_logic_rhs>
    select_logic_con_rhs = "logical conjunction (rhs)";

/// Parse the right-hand-side of a logical disjunction.
x3::rule<select_logic_dis_rhs_class, ast::n_groups::select_multi_logic_rhs>
    select_logic_dis_rhs = "logical disjunction (rhs)";

/// Parse an n-ary logical term (conjunction or disjunction).
x3::rule<select_logic_term_class, ast::n_groups::select_multi_logic>
    select_logic_term = "logical term";

/// Parse a uniform field alias definition.
x3::rule<uniform_field_class, ast::n_groups::uniform_field> uniform_field =
    "uniform field alias";

/// Parse a varying field alias definition.
x3::rule<varying_field_class, ast::n_groups::varying_field> varying_field =
    "varying field alias";

/// Parse a field alias definition.
x3::rule<class field_class, ast::n_groups::field> field = "field alias";

// {{{ definition of the rule id types (..._class)

struct select_atom_class : x3::annotate_on_success {};
struct select_unary_logic_neg_class : x3::annotate_on_success {};
struct select_logic_con_rhs_class : x3::annotate_on_success {};
struct select_logic_dis_rhs_class : x3::annotate_on_success {};
struct select_logic_term_class : x3::annotate_on_success {};
struct uniform_field_class : x3::annotate_on_success {};
struct varying_field_class : x3::annotate_on_success {};

// }}}

} // namespace n_groups

// }}}

struct groups_class;

/// Parse a groups selector.
x3::rule<groups_class, ast::groups> groups = "groups definition";

struct groups_class : x3::annotate_on_success {};

// }}}

// {{{ n_scheme::... rules

namespace n_scheme {

/// Parse an ast::stmt::compute_op value.
struct compute_op_symbols; /* { ... } compute_opt; */

/// Parse an ast::stmt::reduce_op value.
struct reduce_op_symbols; /* { ... } reduce_opt; */

// {{{ declaration of the rule id types (..._class)

struct local_class;
struct compute_class;
struct reduce_class;

struct foreach_neighbor_class;
struct foreach_particle_class;
struct procedure_class;

// }}}

/// Parse a local statement that creates a constant local variable.
x3::rule<local_class, ast::n_scheme::local> local = "local statement";

/// Parse a compute statement that describes a mathematical expression.
x3::rule<compute_class, ast::n_scheme::compute> compute = "compute statement";

/// Parse a reduce statement that describes a "thread-local" mathematical
/// expression.
x3::rule<reduce_class, ast::n_scheme::reduce> reduce = "reduce statement";

/// Parse a statement that is part of a foreach neighbor loop.
x3::rule<
    class foreach_neighbor_statement_class,
    ast::n_scheme::foreach_neighbor::statement>
    foreach_neighbor_statement = "statement (part of a foreach neighbor loop)";

/// Parse a foreach neighbor loop.
x3::rule<foreach_neighbor_class, ast::n_scheme::foreach_neighbor>
    foreach_neighbor = "foreach neighbor loop";

/// Parse a statement that is part of a foreach particle loop.
x3::rule<
    class foreach_particle_statement_class,
    ast::n_scheme::foreach_particle::statement>
    foreach_particle_statement = "statement (part of a foreach particle loop)";

/// Parse a foreach particle loop.
x3::rule<foreach_particle_class, ast::n_scheme::foreach_particle>
    foreach_particle = "foreach particle loop";

// {{{ n_solve::... rules

namespace n_solve {

x3::rule<class statement, ast::n_scheme::n_solve::statement> statement =
    "statement (part of solve { ... })";

class setup_class;
class product_class;
class apply_class;

x3::rule<setup_class, ast::n_scheme::n_solve::setup> setup = "setup";

x3::rule<product_class, ast::n_scheme::n_solve::product> product = "product";

x3::rule<apply_class, ast::n_scheme::n_solve::apply> apply = "apply";

class setup_class : x3::annotate_on_success {};
class product_class : x3::annotate_on_success {};
class apply_class : x3::annotate_on_success {};

} // namespace n_solve

// }}}

class solve_class;

x3::rule<solve_class, ast::n_scheme::solve> solve = "solve";

class solve_class : x3::annotate_on_success {};

/// Parse a statement that is part of a procedure.
x3::rule<class procedure_statement_class, ast::n_scheme::procedure::statement>
    procedure_statement = "statement (part of a procedure)";

/// Parse a procedure.
x3::rule<procedure_class, ast::n_scheme::procedure> procedure = "procedure";

// {{{ definition of the rule id types (..._class)

struct local_class : x3::annotate_on_success {};
struct compute_class : x3::annotate_on_success {};
struct reduce_class : x3::annotate_on_success {};

struct foreach_neighbor_class : x3::annotate_on_success {};
struct foreach_particle_class : x3::annotate_on_success {};
struct procedure_class : x3::annotate_on_success {};

// }}}

} // namespace n_scheme

// }}}

// {{{ declaration of the rule id types (..._class)

struct scheme_class;
struct prtcl_file_class;

// }}}

/// Parse a statement that is part of a scheme.
x3::rule<class statement_class, ast::scheme::statement> scheme_statement =
    "statement (part of a scheme)";

/// Parse a scheme.
x3::rule<scheme_class, ast::scheme> scheme = "scheme";

/// Parse a statement that is part of a .prtcl file.
x3::rule<class prtcl_file_statement_class, ast::prtcl_file::statement>
    prtcl_file_statement = "statement (part of a .prtcl file)";

/// Parse a .prtcl file.
x3::rule<prtcl_file_class, ast::prtcl_file> prtcl_file = ".prtcl file";

// {{{ definition of the rule id types (..._class)

struct scheme_class : x3::annotate_on_success {};
struct prtcl_file_class : error_handler, x3::annotate_on_success {};

// }}}

} // namespace prtcl::gt::parser

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

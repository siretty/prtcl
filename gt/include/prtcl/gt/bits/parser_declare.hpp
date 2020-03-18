#pragma once

#include "../ast.hpp"
#include "prtcl/gt/bits/ast_define.hpp"

#include <boost/spirit/home/x3.hpp>

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

/// Match white space (space, tab, newline, ...) and comments (C/C++ style).
x3::rule<class white_space_class> white_space = "white_space";

/// Parse ast::dtype values.
struct dtype_symbols /* { ... } dtype */;

/// Parse ast::ndtype values.
x3::rule<class ndtype_class, ast::ndtype> ndtype = "ndtype";

/// Parse identifiers as strings.
x3::rule<class identifier_class, string> identifier = "identifier";

namespace n_math {

/// Parse a full mathematical expression.
x3::rule<class expression_class, ast::n_math::expression> expression =
    "mathematical expression";

/// Parse primary (ie. non-arithmetic) expressions.
x3::rule<class primary_class, ast::n_math::expression> primary =
    "mathematical primary expression";

/// Parse mathematical literals.
x3::rule<class literal_class, ast::n_math::literal> literal =
    "mathematical literal";

/// Parse function calls.
x3::rule<class operation_class, ast::n_math::operation> operation =
    "mathematical operation";

/// Parse particle field access.
x3::rule<class field_access_class, ast::n_math::field_access> field_access =
    "field access";

/// Parse a unary negation operation.
x3::rule<class unary_neg_class, ast::n_math::unary_arithmetic> unary_neg =
    "arithmetic negation";

/// Parse the right-hand-side of an addition.
x3::rule<class add_rhs_class, ast::n_math::multi_arithmetic_rhs> add_rhs =
    "arithmetic addition (rhs)";

/// Parse the right-hand-side of a subtraction.
x3::rule<class sub_rhs_class, ast::n_math::multi_arithmetic_rhs> sub_rhs =
    "arithmetic subtraction (rhs)";

/// Parse an n-ary additive operation (ie. addition or subtraction).
x3::rule<class add_term_class, ast::n_math::multi_arithmetic> add_term =
    "arithmetic additive term";

/// Parse the right-hand-side of a multiplication.
x3::rule<class mul_rhs_class, ast::n_math::multi_arithmetic_rhs> mul_rhs =
    "arithmetic multiplication (rhs)";

/// Parse the right-hand-side of a division.
x3::rule<class div_rhs_class, ast::n_math::multi_arithmetic_rhs> div_rhs =
    "arithmetic division (rhs)";

/// Parse an n-ary multiplicative operation (ie. multiplication or division).
x3::rule<class mul_term_class, ast::n_math::multi_arithmetic> mul_term =
    "arithmetic multiplicative term";

} // namespace n_math

namespace n_global {

/// Parse a global field alias definition.
x3::rule<class field_class, ast::n_global::field> field = "global field alias";

} // namespace n_global

/// Parse a global selector.
x3::rule<class global_class, ast::global> global = "global definition";

namespace n_group {

/// Parse a selector expression.
x3::rule<class expression_class, ast::n_group::expression> expression =
    "group selector expression";

/// Parse a primary selector expression.
x3::rule<class primary_class, ast::n_group::expression> primary =
    "primary group selector expression";

struct select_atom_kind_symbols; /* { ... } select_atom_kind; */

/// Parse a type atom.
x3::rule<class select_atom_class, ast::n_group::select_atom> select_atom =
    "select atom";

/// Parse a unary logical negation.
x3::rule<class unary_logic_neg_class, ast::n_group::unary_logic> logic_neg =
    "logical negation";

/// Parse the right-hand-side of a logical conjunction.
x3::rule<class logic_con_rhs_class, ast::n_group::multi_logic_rhs>
    logic_con_rhs = "logical conjunction (rhs)";

/// Parse the right-hand-side of a logical disjunction.
x3::rule<class logic_dis_rhs_class, ast::n_group::multi_logic_rhs>
    logic_dis_rhs = "logical disjunction (rhs)";

/// Parse an n-ary logical term (conjunction or disjunction).
x3::rule<class logic_term_class, ast::n_group::multi_logic> logic_term =
    "logical term";

/// Parse a uniform field alias definition.
x3::rule<class uniform_field_class, ast::n_group::uniform_field> uniform_field =
    "uniform field alias";

/// Parse a varying field alias definition.
x3::rule<class varying_field_class, ast::n_group::varying_field> varying_field =
    "varying field alias";

/// Parse a field alias definition.
x3::rule<class field_class, ast::n_group::field> field = "field alias";

} // namespace n_group

/// Parse a group selector.
x3::rule<class group_class, ast::group> group = "group definition";

namespace n_scheme {

/// Parse an ast::stmt::compute_op value.
struct compute_op_symbols; /* { ... } compute_opt; */

/// Parse an ast::stmt::reduce_op value.
struct reduce_op_symbols; /* { ... } reduce_opt; */

/// Parse a local statement that creates a constant local variable.
x3::rule<class compute_class, ast::n_scheme::local> local = "local statement";

/// Parse a compute statement that describes a mathematical expression.
x3::rule<class compute_class, ast::n_scheme::compute> compute =
    "compute statement";

/// Parse a reduce statement that describes a "thread-local" mathematical
/// expression.
x3::rule<class reduce_class, ast::n_scheme::reduce> reduce = "reduce statement";

/// Parse a statement that is part of a foreach neighbor loop.
x3::rule<
    class foreach_neighbor_statement_class,
    ast::n_scheme::foreach_neighbor::statement>
    foreach_neighbor_statement = "statement (part of a foreach neighbor loop)";

/// Parse a foreach neighbor loop.
x3::rule<class foreach_neighbor_class, ast::n_scheme::foreach_neighbor>
    foreach_neighbor = "foreach neighbor loop";

/// Parse a statement that is part of a foreach particle loop.
x3::rule<
    class foreach_particle_statement_class,
    ast::n_scheme::foreach_particle::statement>
    foreach_particle_statement = "statement (part of a foreach particle loop)";

/// Parse a foreach particle loop.
x3::rule<class foreach_particle_class, ast::n_scheme::foreach_particle>
    foreach_particle = "foreach particle loop";

/// Parse a statement that is part of a procedure.
x3::rule<class procedure_statement_class, ast::n_scheme::procedure::statement>
    procedure_statement = "statement (part of a procedure)";

/// Parse a procedure.
x3::rule<class procedure_class, ast::n_scheme::procedure> procedure =
    "procedure";

} // namespace n_scheme

/// Parse a statement that is part of a scheme.
x3::rule<class statement_class, ast::scheme::statement> scheme_statement =
    "statement (part of a scheme)";

/// Parse a scheme.
x3::rule<class scheme_class, ast::scheme> scheme = "scheme";

/// Parse a statement that is part of a .prtcl file.
x3::rule<class prtcl_file_statement_class, ast::prtcl_file::statement>
    prtcl_file_statement = "statement (part of a .prtcl file)";

/// Parse a .prtcl file.
x3::rule<class prtcl_file_class, ast::prtcl_file> prtcl_file = ".prtcl file";

} // namespace prtcl::gt::parser

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

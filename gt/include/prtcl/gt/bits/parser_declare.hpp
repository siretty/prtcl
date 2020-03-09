#pragma once

#include "../ast.hpp"

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

/// Parse ast::basic_type values.
struct basic_type_symbols /* { ... } basic_type */;

/// Parse ast::storage_qualifier values.
struct storage_qualifier_symbols /* { ... } storage_qualifier */;

/// Parse ast::nd_type values.
x3::rule<class nd_type_class, ast::nd_type> nd_type = "nd_type";

/// Parse identifiers as strings.
x3::rule<class identifier_class, string> identifier = "identifier";

namespace math {

/// Parse a full mathematical expression.
x3::rule<class expression_class, ast::math::expression> expression =
    "expression";

/// Parse primary (ie. non-arithmetic) expressions.
x3::rule<class primary_class, ast::math::expression> primary = "primary";

/// Parse mathematical literals.
x3::rule<class literal_class, ast::math::literal> literal = "literal";

/// Parse named constants.
x3::rule<class constant_class, ast::math::constant> constant = "constant";

/// Parse particle field access.
x3::rule<class field_access_class, ast::math::field_access> field_access =
    "field_access";

/// Parse function calls.
x3::rule<class function_call_class, ast::math::function_call> function_call =
    "function_call";

/// Parse a unary minus operation.
x3::rule<class unary_neg_class, ast::math::unary> unary_neg = "unary_neg";

/// Parse a unary plus operation.
x3::rule<class unary_pos_class, ast::math::unary> unary_pos = "unary_pos";

/// Parse the right-hand-side of an addition.
x3::rule<class add_rhs_class, ast::math::arithmetic_nary_rhs> add_rhs =
    "add_rhs";

/// Parse the right-hand-side of a subtraction.
x3::rule<class sub_rhs_class, ast::math::arithmetic_nary_rhs> sub_rhs =
    "sub_rhs";

/// Parse an n-ary additive operation (ie. addition or subtraction).
x3::rule<class add_term_class, ast::math::arithmetic_nary> add_term =
    "add_term";

/// Parse the right-hand-side of a multiplication.
x3::rule<class mul_rhs_class, ast::math::arithmetic_nary_rhs> mul_rhs =
    "mul_rhs";

/// Parse the right-hand-side of a division.
x3::rule<class div_rhs_class, ast::math::arithmetic_nary_rhs> div_rhs =
    "div_rhs";

/// Parse an n-ary multiplicative operation (ie. multiplication or division).
x3::rule<class mul_term_class, ast::math::arithmetic_nary> mul_term =
    "mul_term";

} // namespace math

namespace init {

/// Parse a field initializer.
x3::rule<class field_class, ast::init::field> field = "init::field";

/// Parse a particle selector initializer.
x3::rule<class particle_selector_class, ast::init::particle_selector>
    particle_selector = "init::particle_selector";

} // namespace init

namespace bits {

/// Parse a statement inside a foreach neighbor loop.
x3::rule<
    class foreach_neighbor_statement_class,
    ast::bits::foreach_neighbor_statement>
    foreach_neighbor_statement = "bits::foreach_neighbor_statement";

/// Parse a statement inside a foreach particle loop.
x3::rule<
    class foreach_particle_statement_class,
    ast::bits::foreach_particle_statement>
    foreach_particle_statement = "bits::foreach_particle_statement";

/// Parse a statement inside a procedure.
x3::rule<class procedure_statement_class, ast::bits::procedure_statement>
    procedure_statement = "bits::procedure_statement";

} // namespace bits

namespace stmt {

/// Parse an ast::stmt::compute_op value.
struct compute_op_symbols;

/// Parse an ast::stmt::reduce_op value.
struct reduce_op_symbols;

/// Parse a let statement that assigns aliases for field and particle selector
/// initializers.
x3::rule<class let_class, ast::stmt::let> let = "stmt::let";

/// Parse a local statement that creates a constant local variable.
x3::rule<class compute_class, ast::stmt::local> local = "stmt::local";

/// Parse a compute statement that describes a mathematical expression.
x3::rule<class compute_class, ast::stmt::compute> compute = "stmt::compute";

/// Parse a reduce statement that describes a "thread-local" mathematical
/// expression.
x3::rule<class reduce_class, ast::stmt::reduce> reduce = "stmt::reduce";

/// Parse a foreach neighbor loop.
x3::rule<class foreach_neighbor_class, ast::stmt::foreach_neighbor>
    foreach_neighbor = "stmt::foreach_neighbor";

/// Parse a foreach particle loop.
x3::rule<class foreach_particle_class, ast::stmt::foreach_particle>
    foreach_particle = "stmt::foreach_particle";

/// Parse a procedure.
x3::rule<class procedure_class, ast::stmt::procedure> procedure =
    "stmt::procedure";

} // namespace stmt

/// Parse a statement inside a prtcl file.
x3::rule<class statement_class, ast::statement> statement = "statement";

/// Parse a while prtcl file.
x3::rule<class prtcl_source_file_class, ast::prtcl_source_file>
    prtcl_source_file = "prtcl_source_file";

} // namespace prtcl::gt::parser

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

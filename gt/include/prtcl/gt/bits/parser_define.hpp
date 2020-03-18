#pragma once

#include "parser_declare.hpp"
#include "prtcl/gt/bits/ast_define.hpp"

// ============================================================
// Rule Definitions
// ============================================================

namespace prtcl::gt::parser {

auto const white_space_def = x3::omit[               //
    +x3::space |                                     //
    (lit("//") >> *(char_ - x3::eol) >> x3::eol) |   //
    (lit("/*") >> *(char_ - lit("*/")) >> lit("*/")) //
];

struct dtype_symbols : x3::symbols<ast::dtype> {
  // {{{ implementation
  dtype_symbols() {
    add                                  //
        ("real", ast::dtype::real)       //
        ("integer", ast::dtype::integer) //
        ("boolean", ast::dtype::boolean);
  }
  // }}}
} dtype;

auto const ndtype_def = //
    dtype >> *('[' >> (x3::uint_ | x3::attr(0u)) >> ']');

auto const identifier_def = x3::lexeme[           //
    (alpha | char_('_')) >> *(alnum | char_('_')) //
];

namespace n_math {

auto const expression_def = add_term;

auto const primary_def =
    '(' >> expression >> ')' | unary_neg | literal | operation | field_access;

auto const literal_def = (                                                    //
    (attr(ast::ndtype_boolean) >> x3::raw[x3::bool_]) |                       //
    (attr(ast::ndtype_real) >> x3::raw[x3::long_double]) |                    //
    (ndtype >> '{' >> x3::raw[x3::lexeme[+(char_ - '}' - ',')] % ','] >> '}') //
);

auto const operation_def = identifier >> -('<' >> ndtype >> '>') >> '(' >>
                           -(expression % ',') >> ')';

auto const field_access_def = identifier >> -('[' >> identifier >> ']');

auto const unary_neg_def = '-' >> attr(ast::op_neg) >> primary;

auto const add_rhs_def = '+' >> attr(ast::op_add) >> mul_term;

auto const sub_rhs_def = '-' >> attr(ast::op_sub) >> mul_term;

auto const add_term_def = mul_term >> *(add_rhs | sub_rhs);

auto const mul_rhs_def = '*' >> attr(ast::op_mul) >> primary;

auto const div_rhs_def = '/' >> attr(ast::op_div) >> primary;

auto const mul_term_def = primary >> *(mul_rhs | div_rhs);

} // namespace n_math

namespace n_global {

auto const field_def =
    lit("field") > identifier >> '=' >> ndtype >> identifier >> ';';

} // namespace n_global

auto const global_def = lit("global") > '{' >> *(n_global::field) >> attr(0) >>
                        '}';

namespace n_group {

auto const expression_def = logic_term;

auto const primary_def = '(' >> expression >> ')' | logic_neg | select_atom;

struct select_atom_kind_symbols : x3::symbols<ast::n_group::select_atom_kind> {
  // {{{ implementation
  select_atom_kind_symbols() {
    add                                     //
        ("type", ast::n_group::select_type) //
        ("tag", ast::n_group::select_tag);
  }
  // }}}
} select_atom_kind;

auto const select_atom_def = select_atom_kind > identifier;

auto const logic_neg_def = lit("not") > attr(ast::op_negation) >> primary;

auto const logic_con_rhs_def = lit("and") >
                               attr(ast::op_conjunction) >> logic_term;

auto const logic_dis_rhs_def = lit("or") >
                               attr(ast::op_disjunction) >> logic_term;

auto const logic_term_def = primary >> *(logic_con_rhs | logic_dis_rhs);

auto const uniform_field_def = lit("uniform") > lit("field") > identifier >
                               '=' > ndtype > identifier > ';';

auto const varying_field_def = lit("varying") > lit("field") > identifier >
                               '=' > ndtype > identifier > ';';

auto const field_def = uniform_field | varying_field;

} // namespace n_group

auto const group_def = lit("group") > identifier > '{' >
                       lit("select") > n_group::expression > ';' >
                       *(n_group::field_def) > '}';

namespace n_scheme {

struct compute_op_symbols : x3::symbols<ast::assign_op> {
  // {{{ implementation
  compute_op_symbols() {
    add                              //
        ("=", ast::op_assign)        //
        ("+=", ast::op_add_assign)   //
        ("-=", ast::op_sub_assign)   //
        ("*=", ast::op_mul_assign)   //
        ("/=", ast::op_div_assign)   //
        ("max=", ast::op_max_assign) //
        ("min=", ast::op_min_assign);
  }
  // }}}
} compute_op;

struct reduce_op_symbols : x3::symbols<ast::assign_op> {
  // {{{ implementation
  reduce_op_symbols() {
    add                              //
        ("+=", ast::op_add_assign)   //
        ("-=", ast::op_sub_assign)   //
        ("*=", ast::op_mul_assign)   //
        ("/=", ast::op_div_assign)   //
        ("max=", ast::op_max_assign) //
        ("min=", ast::op_min_assign);
  }
  // }}}
} reduce_op;

auto const local_def = //
    lit("local") >
    (identifier >> ':' >> ndtype >> '=' >> n_math::expression >> ';');

auto const compute_def = //
    lit("compute") > n_math::field_access >> compute_op >> n_math::expression >>
    ';';

auto const reduce_def = //
    lit("reduce") > n_math::field_access >> reduce_op >> n_math::expression >>
    ';';

auto const foreach_neighbor_statement_def = local | compute | reduce;

auto const foreach_neighbor_def = //
    (lit("foreach") >> identifier >> "neighbor") >
    (identifier >> '{' >>             //
     *(foreach_neighbor_statement) >> //
     '}');

auto const foreach_particle_statement_def =
    local | compute | reduce | foreach_neighbor;

auto const foreach_particle_def = //
    (lit("foreach") >> identifier >> "particle") >
    (identifier >> '{' >>             //
     *(foreach_particle_statement) >> //
     '}');

auto const procedure_statement_def = local | compute | foreach_particle;

auto const procedure_def =                        //
    lit("procedure") > (identifier >> '{' >>      //
                        *(procedure_statement) >> //
                        '}');

} // namespace n_scheme

auto const scheme_statement_def = group | global | n_scheme::procedure;

auto const scheme_def = lit("scheme") > identifier > '{' > *(scheme_statement) >
                        '}';

auto const prtcl_file_statement_def = scheme;

auto const prtcl_file_def = attr("2") >> *(prtcl_file_statement);

} // namespace prtcl::gt::parser

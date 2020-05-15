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

// {{{ n_math::... rules

namespace n_math {

auto const expression_def = add_term;

auto const indexable_def =
    '(' >> expression >> ')' | literal | operation | field_access;

auto const primary_def = component_access | indexable_def | unary_neg;
// auto const primary_def =
//    '(' >> expression >> ')' | unary_neg | literal | operation | field_access
//    | component_access;

auto const literal_def = (                                                    //
    (attr(ast::ndtype_boolean) >> x3::raw[x3::bool_]) |                       //
    (attr(ast::ndtype_real) >> x3::raw[x3::long_double]) |                    //
    (ndtype >> '{' >> x3::raw[x3::lexeme[+(char_ - '}' - ',')] % ','] >> '}') //
);

auto const operation_def = identifier >> -('<' >> ndtype >> '>') >> '(' >>
                           -(expression % ',') >> ')';

auto const field_access_def = identifier >> -('.' >> identifier);

auto const component_access_def = indexable_def >> '[' >> (uint_ % ',') >> ']';

auto const unary_neg_def = '-' >> attr(ast::op_neg) >> primary;

auto const add_rhs_def = '+' >> attr(ast::op_add) >> mul_term;

auto const sub_rhs_def = '-' >> attr(ast::op_sub) >> mul_term;

auto const add_term_def = mul_term >> *(add_rhs | sub_rhs);

auto const mul_rhs_def = '*' >> attr(ast::op_mul) >> primary;

auto const div_rhs_def = '/' >> attr(ast::op_div) >> primary;

auto const mul_term_def = primary >> *(mul_rhs | div_rhs);

} // namespace n_math

// }}}

// {{{ n_global::... rules

namespace n_global {

auto const field_def =
    lit("field") > identifier >> '=' >> ndtype >> identifier >> ';';

} // namespace n_global

// }}}

auto const global_def =              //
    lit("global") > '{' >>           //
    *(n_global::field) >> attr(0) >> //
    '}';

// {{{ n_groups::... rules

namespace n_groups {

auto const select_expression_def = select_logic_term;

auto const select_primary_def = //
    '(' >> select_expression >> ')' | select_logic_neg | select_atom;

struct select_atom_kind_symbols : x3::symbols<ast::n_groups::select_atom_kind> {
  // {{{ implementation
  select_atom_kind_symbols() {
    add                                      //
        ("type", ast::n_groups::select_type) //
        ("tag", ast::n_groups::select_tag);
  }
  // }}}
} select_atom_kind;

auto const select_atom_def = select_atom_kind > identifier;

auto const select_logic_neg_def = //
    lit("not") > attr(ast::op_negation) >> select_primary;

auto const select_logic_con_rhs_def = //
    lit("and") > attr(ast::op_conjunction) >> select_logic_term;

auto const select_logic_dis_rhs_def = //
    lit("or") > attr(ast::op_disjunction) >> select_logic_term;

auto const select_logic_term_def = //
    select_primary >> *(select_logic_con_rhs | select_logic_dis_rhs);

auto const uniform_field_def = //
    lit("uniform") > lit("field") > identifier > '=' > ndtype > identifier >
    ';';

auto const varying_field_def = //
    lit("varying") > lit("field") > identifier > '=' > ndtype > identifier >
    ';';

auto const field_def = uniform_field | varying_field;

} // namespace n_groups

// }}}

auto const groups_def =                                 //
    lit("groups") > identifier > '{' >                  //
    lit("select") > n_groups::select_expression > ';' > //
    *(n_groups::field_def) >                            //
    '}';

// {{{ n_scheme::... rules

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
    (lit("foreach") >> ("particle" >> attr(std::nullopt) | identifier) >>
     "neighbor" >> identifier) > ('{' >>                           //
                                  *(foreach_neighbor_statement) >> //
                                  '}');

auto const foreach_particle_statement_def =
    local | compute | reduce | foreach_neighbor;

auto const foreach_particle_def = //
    (lit("foreach") >> identifier >> "particle") >
    (identifier >> '{' >>             //
     *(foreach_particle_statement) >> //
     '}');

// {{{ n_solve::... rules

namespace n_solve {

auto const statement_def = //
    n_scheme::local | n_scheme::compute | n_scheme::foreach_neighbor;

auto const setup_def =                                           //
    lit("setup") > identifier > lit("into") > identifier > '{' > //
    *(statement) >                                               //
    '}';

auto const product_def =                                        //
    lit("product") > identifier >                               //
    lit("with") > identifier > lit("into") > identifier > '{' > //
    *(statement) >                                              //
    '}';

auto const apply_def =                //
    lit("apply") > identifier > '{' > //
    *(statement) >                    //
    '}';

auto const solve_pcg_def =                                          //
    lit("pcg") > ndtype >                                           //
    lit("over") > identifier > lit("particle") > identifier > '{' > //
    n_solve::setup > /* right_hand_side */                          //
    n_solve::setup > /* guess */                                    //
    n_solve::product > /* preconditioner */                         //
    n_solve::product > /* system */                                 //
    n_solve::apply > /* apply */                                    //
    '}';

} // namespace n_solve

// }}}

auto const solve_def = //
    lit("solve") > n_solve::solve_pcg;
// auto const solve_def =                                              //
//    lit("solve") > identifier > ndtype >                            //
//    lit("over") > identifier > lit("particle") > identifier > '{' > //
//    n_solve::setup > /* right_hand_side */                          //
//    n_solve::setup > /* guess */                                    //
//    n_solve::product > /* preconditioner */                         //
//    n_solve::product > /* system */                                 //
//    n_solve::apply > /* apply */                                    //
//    '}';

auto const procedure_statement_def = local | compute | foreach_particle | solve;

auto const procedure_def =                        //
    lit("procedure") > (identifier >> '{' >>      //
                        *(procedure_statement) >> //
                        '}');

} // namespace n_scheme

// }}}

auto const scheme_statement_def = groups | global | n_scheme::procedure;

auto const scheme_def = //
    lit("scheme") > identifier > '{' > *(scheme_statement) > '}';

auto const prtcl_file_statement_def = scheme;

auto const prtcl_file_def = attr("2") >> *(prtcl_file_statement);

} // namespace prtcl::gt::parser

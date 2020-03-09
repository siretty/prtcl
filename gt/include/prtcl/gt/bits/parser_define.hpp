#pragma once

#include "parser_declare.hpp"

// ============================================================
// Rule Definitions
// ============================================================

namespace prtcl::gt::parser {

auto const white_space_def = x3::omit[               //
    +x3::space |                                     //
    (lit("//") >> *(char_ - x3::eol) >> x3::eol) |   //
    (lit("/*") >> *(char_ - lit("*/")) >> lit("*/")) //
];

struct basic_type_symbols : x3::symbols<ast::basic_type> {
  // {{{ implementation
  basic_type_symbols() {
    add                                       //
        ("real", ast::basic_type::real)       //
        ("integer", ast::basic_type::integer) //
        ("boolean", ast::basic_type::boolean);
  }
  // }}}
} basic_type;

struct storage_qualifier_symbols : x3::symbols<ast::storage_qualifier> {
  // {{{ implementation
  storage_qualifier_symbols() {
    add                                              //
        ("global", ast::storage_qualifier::global)   //
        ("uniform", ast::storage_qualifier::uniform) //
        ("varying", ast::storage_qualifier::varying);
  }
  // }}}
} storage_qualifier;

auto const nd_type_def = //
    basic_type >> *('[' >> (x3::uint_ | x3::attr(0u)) >> ']');

auto const identifier_def = x3::lexeme[           //
    (alpha | char_('_')) >> *(alnum | char_('_')) //
];

namespace math {

auto const expression_def = add_term;

auto const primary_def = '(' >> expression >> ')' | unary_neg | unary_pos |
                         literal | constant | field_access | function_call;

auto const literal_def = (                                   //
    (attr(ast::nd_type::real) >> x3::raw[x3::long_double]) | //
    (attr(ast::nd_type::boolean) >> x3::raw[x3::bool_]) |    //
    (nd_type >> '{' >> x3::raw[x3::lexeme[+(char_ - '}' - ',')] % ','] >>
     '}') //
);

auto const constant_def = identifier >> '<' >> nd_type >> '>';

auto const field_access_def = identifier >> '[' >> identifier >> ']';

auto const function_call_def = identifier >> '(' >> -(expression % ',') >> ')';

auto const add_rhs_def = '+' >> attr(ast::math::arithmetic_op::add) >> mul_term;

auto const sub_rhs_def = '-' >> attr(ast::math::arithmetic_op::sub) >> mul_term;

auto const mul_rhs_def = '*' >> attr(ast::math::arithmetic_op::mul) >> primary;

auto const div_rhs_def = '/' >> attr(ast::math::arithmetic_op::div) >> primary;

auto const add_term_def = mul_term >> *(add_rhs | sub_rhs);

auto const mul_term_def = primary >> *(mul_rhs | div_rhs);

auto const unary_neg_def = '-' >> attr(ast::math::unary_op::neg) >> primary;

auto const unary_pos_def = '+' >> attr(ast::math::unary_op::pos) >> primary;

} // namespace math

namespace init {

auto const field_def =
    lit("field") >> identifier >> ':' >> storage_qualifier >> nd_type;

auto const particle_selector_def =                        //
    lit("particle_selector") >>                           //
    lit("types") >> '{' >> -(identifier % "or") >> '}' >> //
    lit("tags") >> '{' >> -(identifier % "and") >> '}';

} // namespace init

namespace bits {

auto const foreach_neighbor_statement_def =
    stmt::local | stmt::compute | stmt::reduce;

auto const foreach_particle_statement_def =
    stmt::local | stmt::compute | stmt::reduce | stmt::foreach_neighbor;

auto const procedure_statement_def =
    stmt::local | stmt::compute | stmt::foreach_particle;

} // namespace bits

namespace stmt {

struct compute_op_symbols : x3::symbols<ast::stmt::compute_op> {
  // {{{ implementation
  compute_op_symbols() {
    add                                             //
        ("=", ast::stmt::compute_op::assign)        //
        ("+=", ast::stmt::compute_op::add_assign)   //
        ("-=", ast::stmt::compute_op::sub_assign)   //
        ("*=", ast::stmt::compute_op::mul_assign)   //
        ("/=", ast::stmt::compute_op::div_assign)   //
        ("max=", ast::stmt::compute_op::max_assign) //
        ("min=", ast::stmt::compute_op::min_assign);
  }
  // }}}
} compute_op;

struct reduce_op_symbols : x3::symbols<ast::stmt::reduce_op> {
  // {{{ implementation
  reduce_op_symbols() {
    add                                            //
        ("+=", ast::stmt::reduce_op::add_assign)   //
        ("-=", ast::stmt::reduce_op::sub_assign)   //
        ("*=", ast::stmt::reduce_op::mul_assign)   //
        ("/=", ast::stmt::reduce_op::div_assign)   //
        ("max=", ast::stmt::reduce_op::max_assign) //
        ("min=", ast::stmt::reduce_op::min_assign);
  }
  // }}}
} reduce_op;

auto const let_def =                   //
    lit("let") > (identifier >> '=' >> //
                  (init::field | init::particle_selector) >> ';');

auto const local_def = //
    lit("local") >
    (identifier >> ':' >> nd_type >> '=' >> math::expression >> ';');

auto const compute_def = //
    lit("compute") > (identifier >> '[' >> identifier >> ']' >> compute_op >>
                      math::expression >> ';');

auto const reduce_def = //
    lit("reduce") > (identifier >> '[' >> identifier >> ']' >> reduce_op >>
                     math::expression >> ';');

auto const foreach_neighbor_def = //
    (lit("foreach") >> identifier >> "neighbor") >
    (identifier >> '{' >>                 //
     *bits::foreach_neighbor_statement >> //
     '}');

auto const foreach_particle_def = //
    (lit("foreach") >> identifier >> "particle") >
    (identifier >> '{' >>                 //
     *bits::foreach_particle_statement >> //
     '}');

auto const procedure_def =                            //
    lit("procedure") > (identifier >> '{' >>          //
                        *bits::procedure_statement >> //
                        '}');

} // namespace stmt

auto const statement_def = stmt::let | stmt::procedure | lit(';');

auto const prtcl_source_file_def = attr("0.0.1") >> *statement;

} // namespace prtcl::gt::parser

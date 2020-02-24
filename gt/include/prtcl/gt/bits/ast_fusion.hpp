#pragma once

#include "ast_define.hpp"

#include <boost/fusion/adapted.hpp>

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
// }}}

BOOST_FUSION_ADAPT_STRUCT(   //
    prtcl::gt::ast::nd_type, //
    type, shape              //
);

BOOST_FUSION_ADAPT_STRUCT(         //
    prtcl::gt::ast::math::literal, //
    type, value                    //
);

BOOST_FUSION_ADAPT_STRUCT(          //
    prtcl::gt::ast::math::constant, //
    constant_name, constant_type    //
);

BOOST_FUSION_ADAPT_STRUCT(              //
    prtcl::gt::ast::math::field_access, //
    field_name, index_name              //
);

BOOST_FUSION_ADAPT_STRUCT(               //
    prtcl::gt::ast::math::function_call, //
    function_name, arguments             //
);

BOOST_FUSION_ADAPT_STRUCT(                     //
    prtcl::gt::ast::math::arithmetic_nary_rhs, //
    op, rhs                                    //
);

BOOST_FUSION_ADAPT_STRUCT(                 //
    prtcl::gt::ast::math::arithmetic_nary, //
    first_operand, right_hand_sides        //
);

BOOST_FUSION_ADAPT_STRUCT(       //
    prtcl::gt::ast::math::unary, //
    op, operand                  //
);

BOOST_FUSION_ADAPT_STRUCT(          //
    prtcl::gt::ast::init::field,    //
    field_name, storage, field_type //
);

BOOST_FUSION_ADAPT_STRUCT(                   //
    prtcl::gt::ast::init::particle_selector, //
    type_disjunction, tag_conjunction        //
);

BOOST_FUSION_ADAPT_STRUCT(     //
    prtcl::gt::ast::stmt::let, //
    alias_name, initializer    //
);

BOOST_FUSION_ADAPT_STRUCT(                 //
    prtcl::gt::ast::stmt::compute,         //
    field_name, index_name, op, expression //
);

BOOST_FUSION_ADAPT_STRUCT(                 //
    prtcl::gt::ast::stmt::reduce,          //
    field_name, index_name, op, expression //
);

BOOST_FUSION_ADAPT_STRUCT(                         //
    prtcl::gt::ast::stmt::foreach_neighbor,        //
    selector_name, neighbor_index_name, statements //
);

BOOST_FUSION_ADAPT_STRUCT(                         //
    prtcl::gt::ast::stmt::foreach_particle,        //
    selector_name, particle_index_name, statements //
);

BOOST_FUSION_ADAPT_STRUCT(           //
    prtcl::gt::ast::stmt::procedure, //
    procedure_name, statements       //
);

BOOST_FUSION_ADAPT_STRUCT(             //
    prtcl::gt::ast::prtcl_source_file, //
    version, statements                //
);

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

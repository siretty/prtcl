#pragma once

#include "ast_define.hpp"

#include <boost/fusion/adapted.hpp>

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
// }}}

BOOST_FUSION_ADAPT_STRUCT(  //
    prtcl::gt::ast::ndtype, //
    type, shape             //
);

BOOST_FUSION_ADAPT_STRUCT(                 //
    prtcl::gt::ast::n_groups::select_atom, //
    kind, name                             //
);

BOOST_FUSION_ADAPT_STRUCT(                        //
    prtcl::gt::ast::n_groups::select_unary_logic, //
    op, operand                                   //
);

BOOST_FUSION_ADAPT_STRUCT(                            //
    prtcl::gt::ast::n_groups::select_multi_logic_rhs, //
    op, operand                                       //
);

BOOST_FUSION_ADAPT_STRUCT(                        //
    prtcl::gt::ast::n_groups::select_multi_logic, //
    operand, right_hand_sides                     //
);

BOOST_FUSION_ADAPT_STRUCT(                   //
    prtcl::gt::ast::n_groups::uniform_field, //
    alias, type, name                        //
);

BOOST_FUSION_ADAPT_STRUCT(                   //
    prtcl::gt::ast::n_groups::varying_field, //
    alias, type, name                        //
);

BOOST_FUSION_ADAPT_STRUCT(  //
    prtcl::gt::ast::groups, //
    name, select, fields    //
);

BOOST_FUSION_ADAPT_STRUCT(           //
    prtcl::gt::ast::n_global::field, //
    alias, type, name                //
);

BOOST_FUSION_ADAPT_STRUCT(  //
    prtcl::gt::ast::global, //
    fields, __dummy         //
);

BOOST_FUSION_ADAPT_STRUCT(           //
    prtcl::gt::ast::n_math::literal, //
    type, value                      //
);

BOOST_FUSION_ADAPT_STRUCT(                //
    prtcl::gt::ast::n_math::field_access, //
    field, index                          //
);

BOOST_FUSION_ADAPT_STRUCT(             //
    prtcl::gt::ast::n_math::operation, //
    name, type, arguments              //
);

BOOST_FUSION_ADAPT_STRUCT(                    //
    prtcl::gt::ast::n_math::unary_arithmetic, //
    op, operand                               //
);

BOOST_FUSION_ADAPT_STRUCT(                        //
    prtcl::gt::ast::n_math::multi_arithmetic_rhs, //
    op, operand                                   //
);

BOOST_FUSION_ADAPT_STRUCT(                    //
    prtcl::gt::ast::n_math::multi_arithmetic, //
    operand, right_hand_sides                 //
);

BOOST_FUSION_ADAPT_STRUCT(           //
    prtcl::gt::ast::n_scheme::local, //
    name, type, math                 //
);

BOOST_FUSION_ADAPT_STRUCT(             //
    prtcl::gt::ast::n_scheme::compute, //
    left_hand_side, op, math           //
);

BOOST_FUSION_ADAPT_STRUCT(            //
    prtcl::gt::ast::n_scheme::reduce, //
    left_hand_side, op, math          //
);

BOOST_FUSION_ADAPT_STRUCT(                      //
    prtcl::gt::ast::n_scheme::foreach_neighbor, //
    group, index, statements                    //
);

BOOST_FUSION_ADAPT_STRUCT(                      //
    prtcl::gt::ast::n_scheme::foreach_particle, //
    group, index, statements                    //
);

BOOST_FUSION_ADAPT_STRUCT(               //
    prtcl::gt::ast::n_scheme::procedure, //
    name, statements                     //
);

BOOST_FUSION_ADAPT_STRUCT(  //
    prtcl::gt::ast::scheme, //
    name, statements        //
);

BOOST_FUSION_ADAPT_STRUCT(      //
    prtcl::gt::ast::prtcl_file, //
    version, statements         //
);

// {{{ C++ Compiler Diagnostics
#if defined(__GNUG__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
// }}}

#pragma once

#include "prtcl/gt/dsl/component_index.hpp"
#include <prtcl/core/remove_cvref.hpp>

#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/dsl.hpp>

#include <memory>
#include <type_traits>

#include <boost/yap/algorithm.hpp>
#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/print.hpp>

#include <boost/hana/append.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/unpack.hpp>

#include <boost/range/join.hpp>

namespace prtcl::gt::n_dsl_to_ast {

using dsl::expr_kind, dsl::expr_type;

template <typename Callee_, typename... Args_>
using call_expr = expr_type<expr_kind::call, Callee_, Args_...>;

template <typename LHS_, typename RHS_>
using subs_expr = expr_type<expr_kind::subscript, LHS_, RHS_>;

// ============================================================
// Helper Functions
// ============================================================

template <typename C_, typename... As_>
auto split(call_expr<C_, As_...> call_) {
  namespace hana = boost::hana;
  using namespace hana::literals;

  return std::make_pair(
      call_.elements[0_c],
      hana::slice_c<1, hana::length(call_.elements)>(call_.elements));
}

template <typename C_, typename... As_>
auto only_args(call_expr<C_, As_...> call_) {
  namespace hana = boost::hana;
  using namespace hana::literals;

  return hana::slice_c<1, hana::length(call_.elements)>(call_.elements);
}

// ============================================================
// Helper Transforms
// ============================================================

// ------------------------------------------------------------
// Remove Expresion-Ref. Transform
// ------------------------------------------------------------

struct remove_expr_ref_xform {
  template <typename Expr_>
  auto operator()(expr_type<expr_kind::expr_ref, Expr_> ref_) const {
    return boost::yap::deref(ref_);
  }
};

// ============================================================
// DSL-to-AST Transforms
// ============================================================

// ------------------------------------------------------------
// Global Field without Subscript
// ------------------------------------------------------------

struct global_field_to_ast_xform {
  auto operator()(dsl::field_expr<dsl::field_kind::global> term_) const {
    auto field = term_.value();
    return std::make_unique<ast::global_field>(
        field.name, field.dtype, field.shape);
  }
};

// ------------------------------------------------------------
// Uniform and Varying Field with Particle Subscript
// ------------------------------------------------------------

struct particle_field_to_ast_xform {
  auto operator()(
      subs_expr<
          dsl::field_expr<dsl::field_kind::uniform>, dsl::particle_index_expr>
          subs_) const {
    auto node = std::make_unique<ast::particle_subscript>();
    auto field = subs_.left().value();
    node->add_child(ast::uniform_field{field.name, field.dtype, field.shape});
    return node;
  }

  auto operator()(
      subs_expr<
          dsl::field_expr<dsl::field_kind::varying>, dsl::particle_index_expr>
          subs_) const {
    auto node = std::make_unique<ast::particle_subscript>();
    auto field = subs_.left().value();
    node->add_child(ast::varying_field{field.name, field.dtype, field.shape});
    return node;
  }
};

// ------------------------------------------------------------
// Uniform and Varying Field with Neighbor Subscript
// ------------------------------------------------------------

struct neighbor_field_to_ast_xform {
  auto operator()(
      subs_expr<
          dsl::field_expr<dsl::field_kind::uniform>, dsl::neighbor_index_expr>
          subs_) const {
    auto node = std::make_unique<ast::neighbor_subscript>();
    auto field = subs_.left().value();
    node->add_child(ast::uniform_field{field.name, field.dtype, field.shape});
    return node;
  }

  auto operator()(
      subs_expr<
          dsl::field_expr<dsl::field_kind::varying>, dsl::neighbor_index_expr>
          subs_) const {
    auto node = std::make_unique<ast::neighbor_subscript>();
    auto field = subs_.left().value();
    node->add_child(ast::varying_field{field.name, field.dtype, field.shape});
    return node;
  }
};

// ------------------------------------------------------------
// Component Subscript
// ------------------------------------------------------------

struct preprocess_component_index_xform {
  auto operator()(expr_type<
                  expr_kind::comma, dsl::component_index_expr,
                  dsl::component_index_expr>
                      comma_) const {
    auto indices = boost::range::join(
        comma_.left().value().index, comma_.right().value().index);
    return dsl::component_index_expr{
        {nd_index{indices.begin(), indices.end()}}};
  }

  // TODO: support nested levels of comma-expressions
  // TODO: support "raw" unsigned long long indices
};

template <typename LHSXForms_> struct component_index_to_ast_xform {
  template <typename LHS_>
  auto operator()(subs_expr<LHS_, dsl::component_index_expr> subs_) const {
    auto node =
        std::make_unique<ast::component_subscript>(subs_.right().value().index);

    boost::hana::unpack(lhs_xforms, [&subs_, &node](auto &&... xforms_) {
      node->add_child(
          boost::yap::transform_strict(
              subs_.left(), std::forward<decltype(xforms_)>(xforms_)...)
              .release());
    });

    return node;
  }

  LHSXForms_ lhs_xforms;
};

template <typename XFormsTuple_>
auto make_component_subscript_to_ast_xform(XFormsTuple_ &&xforms_tuple_) {
  return component_index_to_ast_xform<core::remove_cvref_t<XFormsTuple_>>{
      std::forward<XFormsTuple_>(xforms_tuple_)};
}

// ------------------------------------------------------------
// Mathematical Expression
// ------------------------------------------------------------

template <typename FieldXForms_> struct math_to_ast_xform {
  // Basic Arithmetic
  // ----------------

  template <
      expr_kind ExprKind_, typename LHS_, typename RHS_,
      typename = std::enable_if_t<boost::hana::in(
          ExprKind_, boost::hana::make_tuple(
                         expr_kind::plus, expr_kind::minus,
                         expr_kind::multiplies, expr_kind::divides))>>
  auto operator()(expr_type<ExprKind_, LHS_, RHS_> expr_) const {
    auto op_name = boost::yap::op_string(ExprKind_);
    auto node = std::make_unique<ast::operation>(op_name);

    boost::hana::for_each(expr_.elements, [this, &node](auto arg) {
      this->_transform(arg, node);
    });

    return node;
  }

  // Constants
  // ---------

  template <typename... Args_> auto operator()(dsl::constant_expr term_) const {
    auto constant = term_.value();
    return std::make_unique<ast::constant>(
        constant.name, constant.dtype, constant.shape);
  }

  // Function Calls
  // --------------

  template <typename... Args_>
  auto operator()(call_expr<dsl::operation_expr, Args_...> call_) const {
    auto [callee, args] = split(call_);
    auto node = std::make_unique<ast::operation>(callee.value().name);

    boost::hana::for_each(
        args, [this, &node](auto arg) { this->_transform(arg, node); });

    return node;
  }

  FieldXForms_ field_xforms;

private:
  template <typename Arg_, typename Node_>
  decltype(auto) _transform(Arg_ &arg_, Node_ &node_) const {
    boost::hana::unpack(
        field_xforms, [this, &arg_, &node_](auto &&... xforms_) {
          auto child = boost::yap::transform_strict(
              arg_, *this, std::forward<decltype(xforms_)>(xforms_)...);
          node_->add_child(child.release());
        });
  }
};

template <typename... FieldXForms_>
auto make_math_to_ast_xform(FieldXForms_ &&... field_xforms_) {
  auto field_xforms_tuple =
      boost::hana::make_tuple(std::forward<FieldXForms_>(field_xforms_)...);
  return math_to_ast_xform<core::remove_cvref_t<decltype(field_xforms_tuple)>>{
      std::move(field_xforms_tuple)};
}

// ------------------------------------------------------------
// Equation
// ------------------------------------------------------------

template <typename LHSXForms_, typename RHSXForms_>
struct equation_to_ast_xform {
  template <
      expr_kind ExprKind_, typename LHS_, typename RHS_,
      typename = std::enable_if_t<boost::hana::in(
          ExprKind_, boost::hana::make_tuple(
                         expr_kind::assign, expr_kind::plus_assign,
                         expr_kind::minus_assign, expr_kind::multiplies_assign,
                         expr_kind::divides_assign))>>
  auto operator()(expr_type<ExprKind_, LHS_, RHS_> expr_) const {
    auto op_name = boost::yap::op_string(ExprKind_);
    auto node = std::make_unique<ast::assignment>(op_name);

    boost::hana::unpack(lhs_xforms, [&expr_, &node](auto &&... xforms_) {
      node->add_child(
          boost::yap::transform_strict(
              expr_.left(), std::forward<decltype(xforms_)>(xforms_)...)
              .release());
    });

    boost::hana::unpack(rhs_xforms, [&expr_, &node](auto &&... xforms_) {
      node->add_child(
          boost::yap::transform_strict(
              expr_.right(), std::forward<decltype(xforms_)>(xforms_)...)
              .release());
    });

    return std::make_unique<ast::equation>(node.release());
  }

  LHSXForms_ lhs_xforms;
  RHSXForms_ rhs_xforms;
};

template <typename LHSXForms_, typename RHSXForms_>
auto make_equation_to_ast_xform(
    LHSXForms_ &&lhs_xforms_, RHSXForms_ &&rhs_xforms_) {
  return equation_to_ast_xform<
      core::remove_cvref_t<LHSXForms_>, core::remove_cvref_t<RHSXForms_>>{
      std::forward<LHSXForms_>(lhs_xforms_),
      std::forward<RHSXForms_>(rhs_xforms_)};
}

// ------------------------------------------------------------
// Foreach Neighbor Loop
// ------------------------------------------------------------

struct foreach_neighbor_to_ast_xform {
  template <typename... Exprs_>
  auto
  operator()(call_expr<dsl::foreach_neighbor_expr, Exprs_...> call_) const {
    auto node = std::make_unique<ast::foreach_particle>();

    boost::hana::for_each(only_args(call_), [&node](auto arg) {
      auto child =
          boost::yap::transform_strict(arg, if_group_type_to_ast_xform{});
      node->add_child(child.release());
    });

    return node;
  }

private:
  struct if_group_type_to_ast_xform {
    template <typename... Exprs_>
    auto operator()(call_expr<dsl::if_group_type_expr, Exprs_...> call_) const {
      auto [callee, args] = split(call_);
      auto node =
          std::make_unique<ast::if_group_type>(callee.value().group_type);

      boost::hana::for_each(args, [&node](auto arg) {
        using boost::hana::make_tuple, boost::hana::append;
        auto lhs_xforms = make_tuple(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{});
        auto child = boost::yap::transform_strict(
            arg,
            make_equation_to_ast_xform(
                append(
                    lhs_xforms,
                    make_component_subscript_to_ast_xform(lhs_xforms)),
                make_tuple(make_math_to_ast_xform(
                    global_field_to_ast_xform{}, particle_field_to_ast_xform{},
                    neighbor_field_to_ast_xform{}))));
        node->add_child(child.release());
      });

      return node;
    }
  };
};

// ------------------------------------------------------------
// Foreach Particle Loop
// ------------------------------------------------------------

struct foreach_particle_to_ast_xform {
  template <typename... Exprs_>
  auto
  operator()(call_expr<dsl::foreach_particle_expr, Exprs_...> call_) const {
    auto node = std::make_unique<ast::foreach_particle>();

    boost::hana::for_each(only_args(call_), [&node](auto arg) {
      auto child =
          boost::yap::transform_strict(arg, if_group_type_to_ast_xform{});
      node->add_child(child.release());
    });

    return node;
  }

private:
  struct if_group_type_to_ast_xform {
    template <typename... Exprs_>
    auto operator()(call_expr<dsl::if_group_type_expr, Exprs_...> call_) const {
      auto [callee, args] = split(call_);
      auto node =
          std::make_unique<ast::if_group_type>(callee.value().group_type);

      boost::hana::for_each(args, [&node](auto arg) {
        using boost::hana::make_tuple, boost::hana::append;
        auto lhs_xforms = make_tuple(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{});
        auto child = boost::yap::transform_strict(
            arg, foreach_neighbor_to_ast_xform{},
            make_equation_to_ast_xform(
                append(
                    lhs_xforms,
                    make_component_subscript_to_ast_xform(lhs_xforms)),
                make_tuple(make_math_to_ast_xform(
                    global_field_to_ast_xform{},
                    particle_field_to_ast_xform{}))));
        node->add_child(child.release());
      });

      return node;
    }
  };
};

// ------------------------------------------------------------
// Procedure
// ------------------------------------------------------------

struct procedure_to_ast_xform {
  template <typename... Exprs_>
  auto operator()(call_expr<dsl::procedure_expr, Exprs_...> call_) const {
    auto [callee, args] = split(call_);
    auto node = std::make_unique<ast::procedure>(callee.value().name);

    boost::hana::for_each(args, [&node](auto arg) {
      using boost::hana::make_tuple, boost::hana::append;
      auto lhs_xforms = make_tuple(global_field_to_ast_xform{});
      auto child = boost::yap::transform_strict(
          arg, foreach_particle_to_ast_xform{},
          make_equation_to_ast_xform(
              append(
                  lhs_xforms,
                  make_component_subscript_to_ast_xform(lhs_xforms)),
              make_tuple(make_math_to_ast_xform(global_field_to_ast_xform{}))));
      node->add_child(child.release());
    });

    return node;
  }
};

} // namespace prtcl::gt::n_dsl_to_ast

namespace prtcl::gt {

template <typename Expr_> auto dsl_to_ast(Expr_ &&expr_) {
  using namespace n_dsl_to_ast;
  return boost::yap::transform_strict(
      boost::yap::transform(
          std::forward<Expr_>(expr_), remove_expr_ref_xform{},
          preprocess_component_index_xform{}),
      procedure_to_ast_xform{});
}

} // namespace prtcl::gt

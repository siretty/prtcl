#pragma once

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
// Temporary AST Nodes
// ============================================================

class dsl_to_ast_reduce final
    : public virtual ast::ast_nary_crtp<dsl_to_ast_reduce> {
  // {{{
public:
  std::string name() const { return _name; }

public:
  char const *ast_node_name() const final { return "dsl_to_ast_reduce"; }

public:
  dsl_to_ast_reduce(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
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

  // Literals
  // --------

  auto operator()(dsl::literal_expr term_) const {
    auto literal = term_.value();
    return std::make_unique<ast::literal>(literal.value, literal.dtype);
  }

  template <
      typename Value_,
      typename = std::enable_if_t<std::is_floating_point<Value_>::value>>
  auto operator()(dsl::term_expr<Value_> term_) const {
    return (*this)(
        dsl::language::rlit(static_cast<long double>(term_.value())));
  }

  template <
      typename Value_,
      typename = std::enable_if_t<std::is_integral<Value_>::value>,
      typename = void>
  auto operator()(dsl::term_expr<Value_> term_) const {
    if constexpr (std::is_same<Value_, bool>::value)
      return (*this)(dsl::language::blit(term_.value()));
    else
      return (*this)(
          dsl::language::ilit(static_cast<long long int>(term_.value())));
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
// DSL-To-AST Reduce
// ------------------------------------------------------------

template <typename XFormsTuple_> struct reduce_to_ast_xform {
  template <typename... Args_>
  auto operator()(call_expr<dsl::reduce_expr, Args_...> call_) const {
    auto [callee, args] = split(call_);
    auto node = std::make_unique<dsl_to_ast_reduce>(callee.value().name);

    boost::hana::for_each(
        args, [this, &node](auto arg) { this->_transform(arg, node); });

    return node;
  }

  XFormsTuple_ field_xforms;

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

template <typename XFormsTuple_>
auto make_reduce_to_ast_xform(XFormsTuple_ &&xforms_tuple_) {
  return reduce_to_ast_xform<core::remove_cvref_t<XFormsTuple_>>{
      std::forward<XFormsTuple_>(xforms_tuple_)};
}

// ------------------------------------------------------------
// Equation and Reduction
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
  std::unique_ptr<ast::ast_node_base>
  operator()(expr_type<ExprKind_, LHS_, RHS_> expr_) const {
    auto op_name = boost::yap::op_string(ExprKind_);

    std::unique_ptr<ast::ast_node_base> lhs;
    boost::hana::unpack(lhs_xforms, [&expr_, &lhs](auto &&... xforms_) {
      lhs = boost::yap::transform_strict(
          expr_.left(), std::forward<decltype(xforms_)>(xforms_)...);
    });

    std::unique_ptr<ast::ast_node_base> rhs;
    boost::hana::unpack(rhs_xforms, [&expr_, &rhs](auto &&... xforms_) {
      rhs = boost::yap::transform_strict(
          expr_.right(), std::forward<decltype(xforms_)>(xforms_)...);
    });

    // TODO: handle assigning to component subscripts

    auto make_assignment = [](auto &lhs, auto &rhs, auto op_name) {
      auto assignment = std::make_unique<ast::assignment>(op_name);
      assignment->add_child(lhs.release());
      assignment->add_child(rhs.release());
      return std::make_unique<ast::equation>(assignment.release());
    };

    if (auto ps = lhs->as_ptr<ast::particle_subscript>()) {
      // varying fields of particles result in equations
      if (ps->children()[0]->as_ptr<ast::varying_field>()) {
        return make_assignment(lhs, rhs, op_name);
      }
    }

    // TODO: handle reductions that are not +=, -=, *=, /=
    if (expr_kind::assign == ExprKind_) {
      if (auto rd = rhs->as_ptr<dsl_to_ast_reduce>()) {
        auto operation = std::make_unique<ast::operation>(rd->name());
        operation->add_child(lhs.release());
        for (auto child : rd->release_children())
          operation->add_child(child);
        return std::make_unique<ast::reduction>(operation.release());
      } else {
        return make_assignment(lhs, rhs, op_name);
      }
    } else {
      auto operation =
          std::make_unique<ast::operation>(std::string{op_name[0]});
      operation->add_child(lhs.release());
      operation->add_child(rhs.release());
      return std::make_unique<ast::reduction>(operation.release());
    }
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
    auto node = std::make_unique<ast::foreach_neighbor>();

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

      boost::hana::for_each(args, [&node](auto stmt) {
        using boost::hana::make_tuple, boost::hana::append;
        // left-hand-side transforms
        auto lhs_xforms_0 = make_tuple(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{});
        auto lhs_xforms = append(
            lhs_xforms_0, make_component_subscript_to_ast_xform(lhs_xforms_0));
        // right-hand-side transforms
        auto rhs_xforms_0 = make_tuple(make_math_to_ast_xform(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{},
            neighbor_field_to_ast_xform{}));
        auto rhs_xforms =
            append(rhs_xforms_0, make_reduce_to_ast_xform(rhs_xforms_0));
        // transform
        auto child = boost::yap::transform_strict(
            stmt, make_equation_to_ast_xform(lhs_xforms, rhs_xforms));
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

      boost::hana::for_each(args, [&node](auto stmt) {
        using boost::hana::make_tuple, boost::hana::append;
        // left-hand-side transforms
        auto lhs_xforms_0 = make_tuple(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{});
        auto lhs_xforms = append(
            lhs_xforms_0, make_component_subscript_to_ast_xform(lhs_xforms_0));
        // right-hand-side transforms
        auto rhs_xforms_0 = make_tuple(make_math_to_ast_xform(
            global_field_to_ast_xform{}, particle_field_to_ast_xform{}));
        auto rhs_xforms =
            append(rhs_xforms_0, make_reduce_to_ast_xform(rhs_xforms_0));
        // execute the transform
        auto child = boost::yap::transform_strict(
            stmt, foreach_neighbor_to_ast_xform{},
            make_equation_to_ast_xform(lhs_xforms, rhs_xforms));
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
  auto e0 = dsl::deep_copy(std::forward<Expr_>(expr_));
  auto e1 = boost::yap::transform(e0, preprocess_component_index_xform{});
  return boost::yap::transform_strict(e1, procedure_to_ast_xform{});
}

} // namespace prtcl::gt

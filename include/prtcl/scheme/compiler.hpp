#pragma once

#include "prtcl/meta/is_any_of.hpp"
#include "prtcl/tags.hpp"
#include <boost/hana/fwd/transform.hpp>
#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/expression.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_subscript_transform.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/scheme/compiler/block.hpp>
#include <prtcl/scheme/compiler/loop.hpp>

#include <prtcl/meta/format_cxx_type.hpp>

#include <type_traits>
#include <utility>
#include <vector>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::scheme {

// forward declarations

template <typename> struct statement;
template <typename> struct is_statement;
template <typename T> constexpr bool is_statement_v = is_statement<T>::value;

template <typename> struct block;
template <typename> struct is_block;
template <typename T> constexpr bool is_block_v = is_block<T>::value;

template <typename> struct loop;
template <typename> struct is_loop;
template <typename T> constexpr bool is_loop_v = is_loop<T>::value;

// statement

template <typename Expr> struct statement {
  // Expr must be a Boost.YAP expression.
  static_assert(boost::yap::is_expr<Expr>::value);

  template <boost::yap::expr_kind Kind>
  static constexpr bool is_assignment_expr_kind_v = meta::is_any_value_of_v<
      boost::yap::expr_kind, Kind, boost::yap::expr_kind::assign,
      boost::yap::expr_kind::plus_assign, boost::yap::expr_kind::minus_assign,
      boost::yap::expr_kind::multiplies_assign,
      boost::yap::expr_kind::divides_assign>;

  // Allow only assignment expressions.
  // TODO: Additionally allow call where the function is a mineq or maxeq tag.
  static_assert(is_assignment_expr_kind_v<Expr::kind>);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the transformed field
    using transformed_expr = decltype(
        boost::yap::transform(std::declval<Expr>(), std::declval<Transform>()));
    // return the resulting accumulation
    return statement<transformed_expr>{
        boost::yap::transform(std::forward<Transform>(transform), expr)};
  }

  Expr expr;
};

template <typename> struct is_statement : std::false_type {};
template <typename T> struct is_statement<statement<T>> : std::true_type {};

// ============================================================

struct nl_item;
struct extern_stmt;

struct outer_stmt;
struct outer_loop;
struct inner_loop;
struct inner_stmt;

struct al_loop;

/// Transforms a source expression into a block, loop, statement tree.
template <typename SchemeInfo> struct source_transform {
  using expr_kind = boost::yap::expr_kind;

  // match: (outer) loop {{{

  template <typename Select, typename... Args>
  auto operator()(boost::yap::expr_tag<expr_kind::call>,
                  prtcl::expr::loop<Select> loop_, Args &&... args_) const {
    (void)(loop_);
    ((void)(args_), ...);
    /*
    using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
    // result stores transformed sub-expressions of all selected groups
    loop<tuple> result;
    // iterate over all (active) groups
    for (size_t gi_a = 0; gi_a < info.get_group_count(); ++gi_a) {
      // store only transformed sub-expressions if the group was selected
      auto &group = info.get_group(gi_a);
      if (loop_.select(group)) {
        result.groups.push_back(gi_a);
        result.expressions.push_back(
            _call_inner(gi_a, std::forward<Args>(args_)...));
      }
    }
    return std::move(result);
    */
  }

  // }}}

  // match: statement {{{

  template <expr_kind Kind, typename... Args>
  auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
    return boost::yap::transform_strict(
        boost::yap::make_expression<Kind>(std::forward<Args>(args_)...),
        nl_stmt{});
  }

  // }}}

  /// Transforms a statement that is not part of a loop.
  struct nl_stmt {
    //
  };

  /// Transforms an active (outer) loop.
  struct outer_loop {
    /// Transforms a statement that is part of an active (outer) loop.
    struct al_stmt {
      //
    };

    /// Transforms a passive (inner) loop.
    struct inner_loop {
      /// Transforms a statement that is part of a passive (inner) loop.
      struct pl_stmt {
        //
      };
    };
  };

  SchemeInfo info;
};

// ============================================================

template <typename Expression> struct expr {
  static_assert(boost::yap::is_expr<Expression>::value);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the transformed expression
    using transformed_expression = decltype(boost::yap::transform(
        std::declval<Expression>(), std::declval<Transform>()));
    // return the resulting expression
    return expr<transformed_expression>{
        boost::yap::transform(expression, std::forward<Transform>(transform))};
  }

  Expression expression;
};

template <typename Field, typename Expr> struct assignment {
  static_assert(prtcl::expr::is_field_v<Field>);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the transformed field
    using transformed_field =
        decltype(std::declval<Transform>()(std::declval<Field>()));
    // return the resulting accumulation
    return assignment<transformed_field, Expr>{
        std::forward<Transform>(transform)(field)};
  }

  Field field;
};

enum class operation { add, sub, mul, div, max, min };

template <typename Field, operation Op, typename Expr> struct accumulation {
  static_assert(prtcl::expr::is_field_v<Field>);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the transformed field
    using transformed_field =
        decltype(std::declval<Transform>()(std::declval<Field>()));
    // return the resulting accumulation
    return accumulation<transformed_field, Op, Expr>{
        std::forward<Transform>(transform)(field)};
  }

  Field field;
};

template <typename Field, operation Op, typename Expr> struct reduction {
  static_assert(prtcl::expr::is_field_v<Field>);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the transformed field
    using transformed_field =
        decltype(std::declval<Transform>()(std::declval<Field>()));
    // return the resulting accumulation
    return accumulation<transformed_field, Op, Expr>{
        std::forward<Transform>(transform)(field)};
  }

  Field field;
};

enum class eq_kind { normal, reduce };
enum class eq_operator { none, add, sub, mul, div, max, min };

template <eq_kind Kind, eq_operator Op, typename LHS, typename RHS> struct eq {
  static_assert(Kind == eq_kind::normal or Op != eq_operator::none);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using lhs_type = decltype(std::declval<Transform>()(std::declval<LHS>()));
    using rhs_type = decltype(std::declval<Transform>()(std::declval<RHS>()));
    // return the resulting reduction
    return eq<Kind, Op, lhs_type, rhs_type>{
        std::forward<Transform>(transform)(lhs),
        std::forward<Transform>(transform)(rhs)};
  }

  LHS lhs;
  RHS rhs;
};

template <typename Scalar, size_t N> class compiler {
public:
  using scheme_data = data::scheme<Scalar, N>;

private:
  struct name_transform {
  public:
    template <typename TT, typename GT, typename V>
    prtcl::expr::field_term<tag::global, TT, GT, size_t>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<tag::global, TT, GT, V> field_) const {
      if (auto index = data.get(tag::global{}, TT{}).get_index(field_.value))
        return {{*index}};
      else
        throw "unknown field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename V>
    prtcl::expr::field_term<KT, TT, tag::active, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<KT, TT, tag::active, V> field_) const {
      auto &group = data.get_group(gi_a);
      if (auto index = group.get(KT{}, TT{}).get_index(field_.value))
        return {{{gi_a, *index}}};
      else
        throw "unknown active field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename V>
    prtcl::expr::field_term<KT, TT, tag::passive, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<KT, TT, tag::passive, V> field_) const {
      auto &group = data.get_group(gi_p);
      if (auto index = group.get(KT{}, TT{}).get_index(field_.value))
        return {{{gi_p, *index}}};
      else
        throw "unknown passive field name (" + field_.value + ")";
    }

    /// Keep terminals by-value.
    template <typename Value>
    boost::yap::terminal<boost::yap::expression, meta::remove_cvref_t<Value>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               Value &&value) const {
      return {{std::forward<Value>(value)}};
    }

    scheme_data &data;
    size_t gi_a, gi_p;
  };

  struct eq_transform {
  public:
    template <typename KT, typename TT, typename GT, typename V, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                    prtcl::expr::field<KT, TT, GT, V> lhs, RHS &&rhs) const {
      return eq<eq_kind::normal, eq_operator::none, decltype(lhs),
                meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
    }

    template <typename KT, typename TT, typename GT, typename V, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                    prtcl::expr::field<KT, TT, GT, V> lhs, RHS &&rhs) const {
      if constexpr (meta::is_any_of_v<KT, tag::varying>)
        return eq<eq_kind::normal, eq_operator::add, decltype(lhs),
                  meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
      else
        return eq<eq_kind::reduce, eq_operator::add, decltype(lhs),
                  meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
    }
  };

  struct split_transform {
    struct inner {
      template <typename Arg> auto _transform(size_t gi_p, Arg &&e0) const {
        auto e1 = boost::yap::transform(std::forward<Arg>(e0),
                                        name_transform{data, gi_a, gi_p});
        auto e2 = boost::yap::transform(e1, eq_transform{});
        return e2;
      }

      template <typename... Args>
      auto _collect(size_t gi_p, Args &&... args_) const {
        return boost::hana::make_tuple(
            _transform(gi_p, boost::yap::as_expr(args_))...);
      }

      template <typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      prtcl::expr::loop<Select> loop_, Args &&... args_) const {
        using tuple = decltype(_collect(0, std::forward<Args>(args_)...));
        loop<block<tuple>> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          auto &group = data.get_group(gi_p);
          if (loop_.select(group)) {
            result.groups.push_back(gi_p);
            result.instances.push_back(
                block<tuple>{_collect(gi_p, std::forward<Args>(args_)...)});
          }
        }
        return std::move(result);
      }

      template <boost::yap::expr_kind Kind, typename... Args>
      auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
        return _transform(gi_a, boost::yap::make_expression<Kind>(
                                    std::forward<Args>(args_)...));
      }

      scheme_data &data;
      size_t gi_a;
    };

    template <typename... Args>
    auto _call_inner(size_t gi_a, Args &&... args_) const {
      return boost::hana::make_tuple(boost::yap::transform(
          boost::yap::as_expr(args_), inner{data, gi_a})...);
    }

    template <typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    prtcl::expr::loop<Select> loop_, Args &&... args_) const {
      using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      loop<block<tuple>> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        auto &group = data.get_group(gi_a);
        if (loop_.select(group)) {
          result.groups.push_back(gi_a);
          result.instances.push_back(
              block<tuple>{_call_inner(gi_a, std::forward<Args>(args_)...)});
        }
      }
      return std::move(result);
    }

    scheme_data &data;
  };

public:
  template <typename E0> auto operator()(E0 &&e0) {
    auto e1 =
        boost::yap::transform(e0, prtcl::expr::field_subscript_transform{});

    display_cxx_type(e1, std::cout);
    boost::yap::print(std::cout, e1);

    auto e2 = boost::yap::transform(e1, split_transform{_data});

    display_cxx_type(e2, std::cout);
    // boost::yap::print(std::cout, e2);

    return std::move(e2);
  }

public:
  compiler(scheme_data &data_) : _data{data_} {}

private:
  scheme_data &_data;
};

} // namespace prtcl::scheme

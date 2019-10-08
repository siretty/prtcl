#pragma once

#include <prtcl/expr/grammar/scalar.hpp>
#include <prtcl/expr/grammar/vector.hpp>

#include <tuple>

#include <boost/fusion/tuple.hpp>
#include <boost/proto/proto.hpp>

namespace prtcl {

template <typename PrepareExprs, typename AccumulateExprs, typename FinishExprs>
struct equation {
  PrepareExprs prepare_expressions;
  AccumulateExprs accumulate_expressions;
  FinishExprs finish_expressions;

  template <typename Functor>
  void for_each_prepare_expression(Functor functor) {
    boost::fusion::for_each(prepare_expressions, functor);
  }

  template <typename Functor>
  void for_each_accumulate_expression(Functor functor) {
    boost::fusion::for_each(accumulate_expressions, functor);
  }

  template <typename Functor> void for_each_finish_expression(Functor functor) {
    boost::fusion::for_each(finish_expressions, functor);
  }

  template <typename Functor> void for_each_expression(Functor functor) {
    for_each_prepare_expression(functor);
    for_each_accumulate_expression(functor);
    for_each_finish_expression(functor);
  }
};

template <typename PrepareExprs, typename AccumulateExprs, typename FinishExprs>
auto make_equation(PrepareExprs &&pe, AccumulateExprs &&ae, FinishExprs &&fe) {
  return equation<PrepareExprs, AccumulateExprs, FinishExprs>{
      std::forward<PrepareExprs>(pe), std::forward<AccumulateExprs>(ae),
      std::forward<FinishExprs>(fe)};
}

template <typename... Exprs>
constexpr auto make_prepare_expressions(Exprs &&... exprs) {
  static_assert((boost::proto::matches<
                     Exprs, boost::proto::or_<expr::VectorAssignment,
                                              expr::ScalarAssignment>>::value &&
                 ...),
                "an expression is not a vector or scalar assignment");
  return boost::fusion::make_tuple(
      boost::proto::deep_copy(std::forward<Exprs>(exprs))...);
}

template <typename... Exprs>
constexpr auto make_accumulate_expressions(Exprs &&... exprs) {
  static_assert(
      (boost::proto::matches<
           Exprs, boost::proto::or_<expr::VectorAccumulation,
                                    expr::ScalarAccumulation>>::value &&
       ...),
      "an expression is not a vector or scalar accumulation");
  return boost::fusion::make_tuple(
      boost::proto::deep_copy(std::forward<Exprs>(exprs))...);
}

template <typename... Exprs>
constexpr auto make_finish_expressions(Exprs &&... exprs) {
  static_assert((boost::proto::matches<
                     Exprs, boost::proto::or_<expr::VectorAssignment,
                                              expr::ScalarAssignment>>::value &&
                 ...),
                "an expression is not a vector or scalar assignment");
  return boost::fusion::make_tuple(
      boost::proto::deep_copy(std::forward<Exprs>(exprs))...);
}

} // namespace prtcl

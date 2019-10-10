#pragma once

#include "../meta/is_any_of.hpp"
#include "../tags.hpp"
#include "field_data.hpp"

#include "../../../sources/tests/prtcl/format_cxx_type.hpp"

#include <iostream>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

struct eval_field_set : boost::proto::callable {
  template <typename T> using rcvr_t = remove_cvref_t<T>;
  template <typename T> using kind_t = typename rcvr_t<T>::kind_tag;
  template <typename T> using type_t = typename rcvr_t<T>::type_tag;
  template <typename T> using group_t = typename rcvr_t<T>::group_tag;

  template <typename> struct result;

  template <typename This, typename FD, typename RHS, typename D>
  struct result<This(FD, RHS, D)> {
    using type = void;
  };

  template <typename FD, typename RHS, typename D>
  typename result<eval_field_set(FD, RHS, D)>::type
  operator()(FD const &field, RHS const &rhs, D data) const {
    static_assert(is_any_of_v<kind_t<FD>, tag::uniform, tag::varying>);
    static_assert(is_any_of_v<group_t<FD>, tag::active, tag::passive>);

    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        field.data.template set<RHS>(field.index, rhs);
      else
        field.data.set(field.index, rhs);
      return;
    }

    size_t index;
    if constexpr (is_any_of_v<group_t<FD>, tag::active>)
      index = data.active;
    if constexpr (is_any_of_v<group_t<FD>, tag::passive>)
      index = data.passive;

    if constexpr (is_any_of_v<kind_t<FD>, tag::varying>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        field.data.template set<RHS>(index, rhs);
      else
        field.data.set(index, rhs);
    }
  }
};

template <typename Scalar, typename Vector>
struct eval_field_get : boost::proto::callable {
  template <typename T> using rcvr_t = remove_cvref_t<T>;
  template <typename T> using kind_t = typename rcvr_t<T>::kind_tag;
  template <typename T> using type_t = typename rcvr_t<T>::type_tag;
  template <typename T> using group_t = typename rcvr_t<T>::group_tag;

  template <typename> struct result;

  template <typename This, typename FD, typename D> struct result<This(FD, D)> {
    static_assert(is_any_of_v<type_t<FD>, tag::scalar, tag::vector>);
    using type = std::conditional_t<is_any_of_v<type_t<FD>, tag::scalar>,
                                    Scalar, Vector>;
  };

  template <typename FD, typename D>
  typename result<eval_field_get(FD, D)>::type operator()(FD const &field,
                                                          D data) const {
    static_assert(is_any_of_v<kind_t<FD>, tag::uniform, tag::varying>);
    static_assert(is_any_of_v<group_t<FD>, tag::active, tag::passive>);

    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        return field.data.template get<Vector>(field.index);
      else
        return field.data.get(field.index);
    }

    size_t index;
    if constexpr (is_any_of_v<group_t<FD>, tag::active>)
      index = data.active;
    if constexpr (is_any_of_v<group_t<FD>, tag::passive>)
      index = data.passive;

    if constexpr (is_any_of_v<kind_t<FD>, tag::varying>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        return field.data.template get<Vector>(index);
      else
        return field.data.get(index);
    }
  }
};

template <typename Scalar, typename Vector>
struct eval_assign_rhs
    : boost::proto::or_<
          boost::proto::when<
              FieldData, boost::proto::_make_terminal(
                             boost::proto::call<eval_field_get<Scalar, Vector>(
                                 boost::proto::_value, boost::proto::_data)>)>,
          // Terminals are stored by value and are not modified in this step.
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_byval(boost::proto::_)>,
          // Recurse into any Non-Terminals.
          boost::proto::nary_expr<
              boost::proto::_,
              boost::proto::vararg<eval_assign_rhs<Scalar, Vector>>>> {};

template <typename Scalar, typename Vector>
struct eval_context
    : boost::proto::callable_context<eval_context<Scalar, Vector> const> {
  size_t active, passive;

  using result_type = void;

  template <typename LHS, typename RHS>
  void operator()(boost::proto::tag::assign, LHS lhs, RHS rhs) const {
    // std::cout << "ASSIGN ";
    // boost::proto::display_expr(lhs);
    // display_cxx_type(lhs, std::cout);

    // std::cout << "TO ";
    // boost::proto::display_expr(rhs);
    // display_cxx_type(rhs, std::cout);

    // assign the left hand side
    eval_field_set{}(
        boost::proto::value(lhs),
        // evaluate the right hand side
        boost::proto::eval(eval_assign_rhs<Scalar, Vector>{}(rhs, 0, *this),
                           *this),
        *this);
  }
};

} // namespace expression
} // namespace prtcl

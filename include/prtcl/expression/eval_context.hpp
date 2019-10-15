#pragma once

#include "../meta/is_any_of.hpp"
#include "../tags.hpp"
#include "field_data.hpp"
#include "function.hpp"

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
    static_assert(is_any_of_v<type_t<FD>, tag::scalar, tag::vector>);
    static_assert(is_any_of_v<group_t<FD>, tag::active, tag::passive>);

    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        return field.data.template get<Vector>(field.index);
      else
        return field.data.get(field.index);
    }

    [[maybe_unused]] size_t index;
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

struct eval_function_tag : boost::proto::callable {
  template <typename T> using rcvr_t = remove_cvref_t<T>;

  template <typename> struct result;

  template <typename This, typename FT, typename Data>
  struct result<This(FT, Data)> {
    static_assert(tag::is_function_tag_v<rcvr_t<FT>>);
    using type = rcvr_t<typename boost::fusion::result_of::at_key<
        decltype(std::declval<Data>().functions), rcvr_t<FT>>::type>;
  };

  template <typename FT, typename Data>
  typename result<eval_function_tag(FT, Data)>::type
  operator()(FT const &, Data &&data) const {
    static_assert(tag::is_function_tag_v<rcvr_t<FT>>);

    return boost::fusion::at_key<rcvr_t<FT>>(
        std::forward<Data>(data).functions);
  }
};

template <typename Scalar, typename Vector>
struct eval_expr
    : boost::proto::or_<
          // Evaluate field data.
          boost::proto::when<
              FieldData, boost::proto::_make_terminal(
                             boost::proto::call<eval_field_get<Scalar, Vector>(
                                 boost::proto::_value, boost::proto::_data)>)>,
          // Evaluate function tags.
          boost::proto::when<Function,
                             boost::proto::_make_terminal(eval_function_tag(
                                 boost::proto::_value, boost::proto::_data))>,
          // Terminals are stored by value and are not modified in this step.
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_byval(boost::proto::_)>,
          // Recurse into any Non-Terminals.
          boost::proto::nary_expr<
              boost::proto::_,
              boost::proto::vararg<eval_expr<Scalar, Vector>>>> {};

template <typename Scalar, typename Vector, typename Functions>
struct eval_context : boost::proto::callable_context<
                          eval_context<Scalar, Vector, Functions> const> {
  size_t active, passive;
  Functions functions;

  using result_type = void;

  // template <typename FunTerm, typename... Args>
  // auto operator()(boost::proto::tag::function, FunTerm, Args &&... args)
  // const {
  //  using key_type =
  //      typename
  //      boost::proto::result_of::value<remove_cvref_t<FunTerm>>::type;
  //  return boost::fusion::at_key<key_type>(functions)(boost::proto::eval(
  //      eval_expr<Scalar, Vector>{}(std::forward<Args>(args), 0, *this),
  //      *this)...);
  //}

  template <typename LHS, typename RHS>
  void operator()(boost::proto::tag::assign, LHS &&lhs, RHS &&rhs) const {
    // assign the left hand side
    eval_field_set{}(boost::proto::value(std::forward<LHS>(lhs)),
                     // evaluate the right hand side
                     boost::proto::eval(eval_expr<Scalar, Vector>{}(
                                            std::forward<RHS>(rhs), 0, *this),
                                        *this),
                     *this);
  }
};

} // namespace expression
} // namespace prtcl

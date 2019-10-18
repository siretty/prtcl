#pragma once

#include "field.hpp"
#include "loop.hpp"

#include <iomanip>
#include <ostream>
#include <type_traits>

#include <cstddef>

#include <boost/hana.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct text_transform {
  // ============================================================
  // loop
  // ============================================================

  template <typename S, typename Arg0, typename... Args>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                  loop<tag::active, S>, Arg0 &&arg0, Args &&... args) const {
    os << "for_each(";
    boost::yap::transform(boost::yap::as_expr(std::forward<Arg0>(arg0)), *this);
    boost::hana::for_each(
        boost::hana::make_tuple(std::forward<Args>(args)...), [this](auto &&e) {
          os << ", ";
          boost::yap::transform(
              boost::yap::as_expr(std::forward<decltype(e)>(e)), *this);
        });
  }

  template <typename S, typename Arg0, typename... Args>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                  loop<tag::passive, S>, Arg0 &&arg0, Args &&... args) const {
    os << "for_each_neighbour(";
    boost::yap::transform(boost::yap::as_expr(std::forward<Arg0>(arg0)), *this);
    boost::hana::for_each(
        boost::hana::make_tuple(std::forward<Args>(args)...), [this](auto &&e) {
          os << ", ";
          boost::yap::transform(
              boost::yap::as_expr(std::forward<decltype(e)>(e)), *this);
        });
  }

  // ============================================================
  // =, +=, -=
  // ============================================================

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                  LHS &&lhs, RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " = ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                  LHS &&lhs, RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " += ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus_assign>,
                  LHS &&lhs, RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " -= ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  // ============================================================
  // +, -, *, /
  // ============================================================

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus>, LHS &&lhs,
                  RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " + ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus>, LHS &&lhs,
                  RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " - ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::multiplies>,
                  LHS &&lhs, RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " * ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  template <typename LHS, typename RHS>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::divides>,
                  LHS &&lhs, RHS &&rhs) const {
    boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)), *this);
    os << " / ";
    boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)), *this);
  }

  // ============================================================
  // terminals
  // ============================================================

  template <typename KT, typename TT, typename GT, typename AT, typename V>
  void operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                  field<KT, TT, GT, AT, V> const &field_) const {
    os << "field(kind=\"" << KT{} << "\", type=\"" << TT{} << "\", group=\""
       << GT{} << "\", name="
       << std::quoted(boost::lexical_cast<std::string>(field_.value)) << ")";
  }

  template <typename Value>
  std::enable_if_t<!is_field<remove_cvref_t<Value>>::value>
  operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
             Value &&value) const {
    os << std::forward<Value>(value);
  }

  std::ostream &os;
};

} // namespace prtcl::expr

#pragma once

#include "common.hpp"

#include <algorithm>
#include <initializer_list>
#include <limits>

#include <cstddef>

#include <Eigen/Eigen>

namespace prtcl::rt {

namespace n_eigen {

// {{{ select_dtype_type_t

template <typename, nd_dtype> struct select_dtype_type;

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::real> {
  using type = typename TypePolicy_::real;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::integer> {
  using type = typename TypePolicy_::integer;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::boolean> {
  using type = typename TypePolicy_::boolean;
};

template <typename TypePolicy_, nd_dtype DType_>
using select_dtype_type_t =
    typename select_dtype_type<TypePolicy_, DType_>::type;

// }}}

// {{{ select_eigen_[d]type_t

template <typename, size_t...> struct select_eigen_type;

template <typename T_> struct select_eigen_type<T_> { using type = T_; };

template <typename T_, size_t N_> struct select_eigen_type<T_, N_> {
  using type = Eigen::Matrix<T_, static_cast<int>(N_), static_cast<int>(1)>;
};

template <typename T_, size_t M_, size_t N_>
struct select_eigen_type<T_, M_, N_> {
  using type = Eigen::Matrix<T_, static_cast<int>(M_), static_cast<int>(N_)>;
};

template <typename T_, size_t... Ns_>
using select_eigen_type_t = typename select_eigen_type<T_, Ns_...>::type;

template <typename TypePolicy_, nd_dtype DType_, size_t... Ns_>
using select_eigen_dtype_t =
    select_eigen_type_t<select_dtype_type_t<TypePolicy_, DType_>, Ns_...>;

// }}}

} // namespace n_eigen

template <typename TypePolicy_> struct eigen_math_policy {
  using real = typename TypePolicy_::real;
  using integer = typename TypePolicy_::integer;
  using boolean = typename TypePolicy_::boolean;

public:
  template <nd_dtype DType_>
  using dtype_t = typename TypePolicy_::template dtype_t<DType_>;

public:
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_t = n_eigen::select_eigen_dtype_t<TypePolicy_, DType_, Ns_...>;

public:
  template <size_t... Ns_>
  using nd_real_t = n_eigen::select_eigen_type_t<real, Ns_...>;

  template <size_t... Ns_>
  using nd_integer_t = n_eigen::select_eigen_type_t<integer, Ns_...>;

  template <size_t... Ns_>
  using nd_boolean_t = n_eigen::select_eigen_type_t<boolean, Ns_...>;

public:
  struct operations {
    template <typename LHS_, typename RHS_>
    static decltype(auto) dot(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_).dot(std::forward<RHS_>(rhs_));
    }

    template <typename Arg_> static decltype(auto) norm(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).norm();
    }

    template <typename Arg_> static decltype(auto) norm_squared(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).normSquared();
    }

    template <typename Arg_> static decltype(auto) normalized(Arg_ &&arg_) {
      // TODO: Eigen-specific: if the norm(arg_) is very small, returns arg_
      return std::forward<Arg_>(arg_).normalized();
    }

    template <typename... Args_> static decltype(auto) max(Args_ &&... args_) {
      // TODO: support coefficient-wise max for non-real / non-scalars
      return std::max(
          std::initializer_list<real>{std::forward<Args_>(args_)...});
    }

    template <typename... Args_> static decltype(auto) min(Args_ &&... args_) {
      // TODO: support coefficient-wise max for non-real / non-scalars
      return std::min(
          std::initializer_list<real>{std::forward<Args_>(args_)...});
    }
  };

public:
  struct constants {
    template <nd_dtype DType_, size_t... Ns_> static decltype(auto) zeros() {
      if constexpr (sizeof...(Ns_))
        return nd_dtype_t<DType_, Ns_...>::Zero();
      else
        return static_cast<dtype_t<DType_>>(0);
    }

    template <nd_dtype DType_, size_t... Ns_> static decltype(auto) ones() {
      if constexpr (sizeof...(Ns_))
        return nd_dtype_t<DType_, Ns_...>::Ones();
      else
        return static_cast<dtype_t<DType_>>(1);
    }

    template <nd_dtype DType_, size_t... Ns_>
    static decltype(auto) most_negative() {
      static_assert(
          nd_dtype::real == DType_ or nd_dtype::integer == DType_, "");
      using type = dtype_t<DType_>;
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<type>::lowest();
      else
        return n_eigen::select_eigen_type_t<type, Ns_...>::Constant(
            most_negative<DType_>());
    }

    template <nd_dtype DType_, size_t... Ns_>
    static decltype(auto) most_positive() {
      static_assert(
          nd_dtype::real == DType_ or nd_dtype::integer == DType_, "");
      using type = dtype_t<DType_>;
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<type>::max();
      else
        return n_eigen::select_eigen_type_t<type, Ns_...>::Constant(
            most_positive<DType_>());
    }

    template <nd_dtype DType_, size_t... Ns_>
    static decltype(auto) positive_infinity() {
      static_assert(DType_ == nd_dtype::real, "");
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<real>::infinity();
      else
        return n_eigen::select_eigen_type_t<real, Ns_...>::Constant(
            positive_infinity<DType_>());
    }

    template <nd_dtype DType_, size_t... Ns_>
    static decltype(auto) negative_infinity() {
      static_assert(DType_ == nd_dtype::real, "");
      if constexpr (0 == sizeof...(Ns_))
        return -std::numeric_limits<real>::infinity();
      else
        return n_eigen::select_eigen_type_t<real, Ns_...>::Constant(
            positive_infinity<DType_>());
    }

    template <nd_dtype DType_, size_t... Ns_> static decltype(auto) identity() {
      static_assert(DType_ == nd_dtype::real and 2 == sizeof...(Ns_), "");
      return n_eigen::select_eigen_type_t<real, Ns_...>::Identity();
    }
  };
};

} // namespace prtcl::rt

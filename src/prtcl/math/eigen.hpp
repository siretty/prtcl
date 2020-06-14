#ifndef PRTCL_MATH_EIGEN_HPP
#define PRTCL_MATH_EIGEN_HPP

#include <prtcl/cxx.hpp>

#include <Eigen/Eigen>

#include <algorithm>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

#include <cstddef>

// TODO: adapt names to coding style (PascalCase)
//       -> this requires translating the names of .prtcl function calls

// TODO: wrap into custom templated types that carry information about an object
//       being a scalar / vector / matrix or worse

namespace prtcl::math {

// ============================================================================
// Types and Aliases
// ============================================================================

namespace detail {

template <typename, size_t... N>
struct SelectTensor {
  static_assert(
      sizeof...(N) <= 2, "The math implementation based on the Eigen library "
                         "currently only supports tensors of up to rank 2.");
};

template <typename T, size_t Rows, size_t Cols>
struct SelectTensor<T, Rows, Cols> {
  using Type = Eigen::Matrix<T, static_cast<int>(Rows), static_cast<int>(Cols)>;
};

template <typename T, size_t Rows>
struct SelectTensor<T, Rows> {
  using Type = Eigen::Matrix<T, static_cast<int>(Rows), 1>;
};

template <typename T>
struct SelectTensor<T> {
  using Type = T;
};

template <typename T, size_t R>
struct SelectDynamicTensor {
  static_assert(
      R <= 2, "The math implementation based on the Eigen library "
              "currently only supports tensors of up to rank 2.");
};

template <typename T>
struct SelectDynamicTensor<T, 2> {
  using Type = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
};

template <typename T>
struct SelectDynamicTensor<T, 1> {
  using Type = Eigen::Matrix<T, Eigen::Dynamic, 1>;
};

template <typename T>
struct SelectDynamicTensor<T, 0> {
  using Type = T;
};

}; // namespace detail

template <typename T, size_t... N>
using Tensor = typename detail::SelectTensor<T, N...>::Type;

template <typename T, size_t R>
using DynamicTensor = typename detail::SelectDynamicTensor<T, R>::Type;

namespace detail {

// from: https://stackoverflow.com/a/51472601/9686644

template <typename Derived>
constexpr bool IsScalar(Eigen::EigenBase<Derived> const *) {
  return false;
}

constexpr bool IsScalar(void const *) { return true; }

template <typename T>
constexpr bool kIsScalar =
    IsScalar(static_cast<std::add_pointer_t<cxx::remove_cvref_t<T>>>(nullptr));

} // namespace detail

template <typename T, typename... U>
constexpr bool IsScalar() {
  return (detail::kIsScalar<T> and ... and detail::kIsScalar<U>());
}

// template <typename T, size_t... N>
// ndtype_t<DType_, Ns_...>
//    from_nested_array(core::narray_t<ndtype_t<DType_>, Ns_...> value_) {
// if constexpr (sizeof...(Ns_) == 0)
// return value_;
// else {
// using result_type = ndtype_t<DType_, Ns_...>;
// return result_type{
// Eigen::Map<result_type>{core::narray_data(value_)}.transpose()};
//}
//}

// ============================================================================
// Operations
// ============================================================================

template <typename LHS, typename RHS>
decltype(auto) dot(LHS &&lhs, RHS &&rhs) {
  // TODO: ensure they are vectors (not just not scalars)
  static_assert(not(IsScalar<LHS>() or IsScalar<RHS>()));
  return std::forward<LHS>(lhs).dot(std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
decltype(auto) cross(LHS &&lhs, RHS &&rhs) {
  // TODO: ensure they are vectors (not just not scalars)
  static_assert(not(IsScalar<LHS>() or IsScalar<RHS>()));
  return std::forward<LHS>(lhs).cross(std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS>
decltype(auto) matmul(LHS &&lhs, RHS &&rhs) {
  // TODO: ensure they are matrices (not just not scalars)
  static_assert(not(IsScalar<LHS>() or IsScalar<RHS>()));
  return std::forward<LHS>(lhs) * std::forward<RHS>(rhs);
}

template <typename Arg>
decltype(auto) trace(Arg &&arg) {
  return std::forward<Arg>(arg).trace();
}

template <typename Arg>
decltype(auto) transpose(Arg &&arg) {
  return std::forward<Arg>(arg).transpose();
}

template <typename Arg>
decltype(auto) diagonal(Arg &&arg) {
  return std::forward<Arg>(arg).diagonal();
}

template <typename Arg>
decltype(auto) norm(Arg &&arg) {
  if constexpr (IsScalar<Arg>())
    return std::abs(std::forward<Arg>(arg));
  else
    return std::forward<Arg>(arg).norm();
}

template <typename Arg>
decltype(auto) norm_squared(Arg &&arg) {
  return std::forward<Arg>(arg).squaredNorm();
}

template <typename Arg>
decltype(auto) normalized(Arg &&arg) {
  // TODO: Eigen-specific: if the norm(arg_) is very small, returns arg_
  return std::forward<Arg>(arg).normalized();
}

template <typename LHS, typename RHS>
decltype(auto) cmul(LHS &&lhs, RHS &&rhs) {
  return (std::forward<LHS>(lhs).array() * std::forward<RHS>(rhs).array())
      .matrix();
}

template <typename LHS, typename RHS>
decltype(auto) cdiv(LHS &&lhs, RHS &&rhs) {
  return (std::forward<LHS>(lhs).array() * std::forward<RHS>(rhs).array())
      .matrix();
}

template <typename Arg>
decltype(auto) maximum_component(Arg &&arg) {
  return std::forward<Arg>(arg).maxCoeff();
}

template <typename Arg>
decltype(auto) minimum_component(Arg &&arg) {
  return std::forward<Arg>(arg).minCoeff();
}

template <typename Arg>
decltype(auto) sum(Arg &&arg) {
  return std::forward<Arg>(arg).sum();
}

template <typename Arg>
decltype(auto) product(Arg &&arg) {
  return std::forward<Arg>(arg).prod();
}

/// Compute the inverse.
template <typename Arg>
decltype(auto) invert(Arg &&a_) {
  return std::forward<Arg>(a_).inverse();
}

/// Compute the Penrose-Moore pseudo inverse.
template <typename Arg>
decltype(auto) invert_pm(Arg &&a_) {
  return std::forward<Arg>(a_)
      .completeOrthogonalDecomposition()
      .pseudoInverse()
      .eval();
}

/// Solve a positive- or negative-semi-definite linear system of equation.
template <typename Matrix, typename RHS>
decltype(auto) solve_sd(Matrix &&a_, RHS &&b_) {
  return std::forward<Matrix>(a_).ldlt().solve(std::forward<RHS>(b_)).eval();
}

template <typename Arg>
decltype(auto) smoothstep(Arg &&arg) {
  using type = std::remove_reference_t<std::remove_const_t<decltype(arg)>>;
  auto const x = std::min(std::max(std::forward<Arg>(arg), type{0}), type{1});
  return x * x * (3 - 2 * x);
}

// TODO: really neccessary?
template <typename Arg, typename Eps>
auto reciprocal_or_zero(Arg &&arg, Eps &&eps) {
  using type = std::remove_reference_t<std::remove_const_t<decltype(arg)>>;
  if (auto const x = std::forward<Arg>(arg);
      std::abs(x) > std::forward<Eps>(eps))
    return 1 / x;
  else
    return type{0};
}

/*

/// Unit Step / Heaviside Step function (left-continuous variant).
///   x \mapsto
///       1   if x > eps
///       0   otherwise
template<typename Eps_, typename... Arg_>
static real unit_step_l(Eps_ &&eps_, Arg_ &&... arg) {
  real const eps = static_cast<real>(std::forward<Eps_>(eps_));
  if (((eps < static_cast<real>(std::forward<Arg_>(arg))) or ...))
    return real{1};
  else
    return real{0};
}

/// Unit Step / Heaviside Step function (right-continuous variant).
///   x \mapsto
///       1   if x >= eps
///       0   otherwise
template<typename Eps_, typename... Arg_>
static real unit_step_r(Eps_ &&eps_, Arg_ &&... arg) {
  real const eps = static_cast<real>(std::forward<Eps_>(eps_));
  if (((eps <= static_cast<real>(std::forward<Arg_>(arg))) or ...))
    return real{1};
  else
    return real{0};
}

template<typename Arg_>
static ndtype_t<dtype::real, 3, 3>
cross_product_matrix_from_vector(Arg_ const &arg) {
  real const zero = 0;
  return narray<dtype::real, 3, 3>(
      {{{zero, -arg[2], arg[1]}, {arg[2], zero, -arg[0]}, {-arg[1], arg[0],
zero},}});
}

template<typename Arg_>
static ndtype_t<dtype::real, 3>
vector_from_cross_product_matrix(Arg_ const &arg) {
  // TODO: assert that arg is a 3x3 matrix
  return {arg(2, 1), arg(0, 2), arg(1, 0)};
}

*/

// ============================================================================
// Constants
// ============================================================================

template <typename T, size_t... N>
decltype(auto) zeros() {
  static_assert(sizeof...(N) <= 2, "rank > 2 is not unsupported");
  if constexpr (sizeof...(N) >= 1)
    return Tensor<T, N...>::Zero();
  else
    return T{0};
}

template <typename T, size_t... N>
decltype(auto) ones() {
  static_assert(sizeof...(N) <= 2, "rank > 2 is not unsupported");
  if constexpr (sizeof...(N) >= 1)
    return Tensor<T, N...>::Ones();
  else
    return T{1};
}

template <typename T, size_t... N, typename Arg>
decltype(auto) Constant(Arg &&value) {
  if constexpr (0 == sizeof...(N))
    return static_cast<T>(std::forward<Arg>(value));
  else
    return Tensor<T, N...>::Constant(Constant<T>(std::forward<Arg>(value)));
}

template <typename T, size_t... N>
decltype(auto) identity() {
  static_assert(sizeof...(N) == 2, "unsupported: rank must be exactly 2");
  return Tensor<T, N...>::Identity();
}

template <typename Arg>
decltype(auto) At(Arg &&arg, cxx::span<size_t const> index) {
  if constexpr (IsScalar<Arg>()) {
    assert(index.size() == 0);
    return std::forward<Arg>(arg);
  } else {
    assert(index.size() >= 1 and index.size() <= 2);
    switch (index.size()) {
    case 1:
      return std::forward<Arg>(arg)(static_cast<Eigen::Index>(index[0]));
    case 2:
      return std::forward<Arg>(arg)(
          static_cast<Eigen::Index>(index[0]),
          static_cast<Eigen::Index>(index[1]));
    default:
      std::terminate();
    }
  }
}

template <typename T>
std::string ToString(T const &t) {
  if constexpr (IsScalar<T>())
    return std::to_string(t);
  else {
    std::ostringstream ss;
    ss << '[';
    for (Eigen::Index row = 0; row < t.rows(); ++row) {
      if (0 < row)
        ss << ", ";
      if (t.cols() > 1)
        ss << '[';
      for (Eigen::Index col = 0; col < t.cols(); ++col) {
        if (0 < col)
          ss << ", ";
        ss << t(row, col);
      }
      if (t.cols() > 1)
        ss << ']';
    }
    ss << ']';
    return ss.str();
  }
}

} // namespace prtcl::math

#endif // PRTCL_MATH_EIGEN_HPP

#pragma once

#include "common.hpp"

#include <prtcl/rt/math/math_policy.hpp>

#include <prtcl/core/identity.hpp>
#include <prtcl/core/log/logger.hpp>
#include <prtcl/core/narray.hpp>

#include <algorithm>
#include <initializer_list>
#include <limits>

#include <cmath>
#include <cstddef>

#include <Eigen/Eigen>

#include <omp.h>

// {{{ implementation details

namespace prtcl::rt::n_eigen {

template <typename, typename> class SystemMatrix;
template <size_t, typename, typename> class BlockSystemMatrix;

} // namespace prtcl::rt::n_eigen

namespace Eigen {
namespace internal {

template <typename MathPolicy_, typename ProductF_>
struct traits<prtcl::rt::n_eigen::SystemMatrix<MathPolicy_, ProductF_>>
    : public Eigen::internal::traits<
          Eigen::SparseMatrix<typename MathPolicy_::type_policy::real>> {};

template <size_t BlockSize_, typename MathPolicy_, typename ProductF_>
struct traits<
    prtcl::rt::n_eigen::BlockSystemMatrix<BlockSize_, MathPolicy_, ProductF_>>
    : public Eigen::internal::traits<
          Eigen::SparseMatrix<typename MathPolicy_::type_policy::real>> {};

} // namespace internal
} // namespace Eigen

// }}}

namespace prtcl::rt {

namespace n_eigen {

// {{{ select_dtype_type_t

template <typename, dtype> struct select_dtype_type;

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, dtype::real> {
  using type = typename TypePolicy_::real;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, dtype::integer> {
  using type = typename TypePolicy_::integer;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, dtype::boolean> {
  using type = typename TypePolicy_::boolean;
};

template <typename TypePolicy_, dtype DType_>
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

template <typename TypePolicy_, dtype DType_, size_t... Ns_>
using select_eigen_dtype_t =
    select_eigen_type_t<select_dtype_type_t<TypePolicy_, DType_>, Ns_...>;

// }}}

// {{{ eigen_extent

template <typename, size_t> struct eigen_extent;

template <typename EigenType_> struct eigen_extent<EigenType_, 0> {
  static constexpr size_t value =
      static_cast<size_t>(EigenType_::RowsAtCompileTime);
};

template <typename EigenType_> struct eigen_extent<EigenType_, 1> {
  static constexpr size_t value =
      static_cast<size_t>(EigenType_::ColsAtCompileTime);
};

template <typename EigenType_, size_t Dimension_>
constexpr size_t eigen_extent_v = eigen_extent<EigenType_, Dimension_>::value;

// }}}

// {{{ SystemMatrix

/// Adaptor for a linear system specified by a functor following
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <typename MathPolicy_, typename ProductF_>
class SystemMatrix
    : public Eigen::EigenBase<SystemMatrix<MathPolicy_, ProductF_>> {
  using self_type = SystemMatrix<MathPolicy_, ProductF_>;

  using math_policy = MathPolicy_;
  using type_policy = typename math_policy::type_policy;

public:
  using Scalar = typename type_policy::real;
  using RealScalar = typename type_policy::real;
  using StorageIndex = int;

  enum {
    ColsAtCompileTime = Eigen::Dynamic,
    MaxColsAtCompileTime = Eigen::Dynamic,
    IsRowMajor = false
  };

  Eigen::Index outerSize() const { return static_cast<Eigen::Index>(_size); }

  Eigen::Index rows() const { return outerSize(); }
  Eigen::Index cols() const { return outerSize(); }

  template <typename Rhs>
  Eigen::Product<self_type, Rhs, Eigen::AliasFreeProduct>
  operator*(const Eigen::MatrixBase<Rhs> &x) const {
    return Eigen::Product<self_type, Rhs, Eigen::AliasFreeProduct>(
        *this, x.derived());
  }

  SystemMatrix(size_t size_, ProductF_ functor_)
      : _size{size_}, _functor{functor_} {}

public:
  size_t system_size() const { return _size; }
  auto const &system_functor() const { return _functor; }

private:
  size_t _size;
  ProductF_ _functor;
};

// }}}

// {{{ make_system_matrix(size, functor)

template <typename MathPolicy_, typename ProductF_>
auto make_system_matrix(size_t size, ProductF_ &&functor) {
  return SystemMatrix<
      MathPolicy_, std::remove_const_t<std::remove_reference_t<ProductF_>>>{
      size, std::forward<ProductF_>(functor)};
}

// }}}

// {{{ DiagonalMatrix

template <typename SystemMatrix_, typename DiagonalF_> class DiagonalMatrix {
  using system_matrix_type = SystemMatrix_;

public:
  using StorageIndex = typename system_matrix_type::StorageIndex;
  using Scalar = typename system_matrix_type::Scalar;

  enum {
    ColsAtCompileTime = Eigen::Dynamic,
    MaxColsAtCompileTime = Eigen::Dynamic
  };

  DiagonalMatrix() {}

  void init(size_t size_, DiagonalF_ *functor_) {
    _size = size_;
    _functor = functor_;
    _diagonal.resize(_size);
  }

  Eigen::Index rows() const { return _size; }
  Eigen::Index cols() const { return _size; }

  Eigen::ComputationInfo info() { return Eigen::Success; }

  template <typename MatType> DiagonalMatrix &analyzePattern(const MatType &) {
    // nothing to do
    return *this;
  }

  template <typename MatType> DiagonalMatrix &factorize(const MatType &) {
    // nothing to do
    return *this;
  }

  template <typename MatType> DiagonalMatrix &compute(const MatType &) {
#pragma omp parallel
    {
#pragma omp for
      for (size_t i = 0; i < _size; i++) {
        auto ii = static_cast<Eigen::Index>(i);
        _diagonal(ii) = 1 / (*_functor)(i);
      }
    }
    return *this;
  }

  template <typename RightHandSide_, typename Column_>
  void
  _solve_impl(const RightHandSide_ &right_hand_side, Column_ &column) const {
    column = _diagonal.array() * right_hand_side.array();
  }

  template <typename RightHandSide_>
  inline const Eigen::Solve<DiagonalMatrix, RightHandSide_>
  solve(const Eigen::MatrixBase<RightHandSide_> &b) const {
    return Eigen::Solve<DiagonalMatrix, RightHandSide_>(*this, b.derived());
  }

private:
  size_t _size;
  DiagonalF_ *_functor = nullptr;
  Eigen::Matrix<Scalar, Eigen::Dynamic, 1> _diagonal;
};

// }}}

// {{{ BlockSystemMatrix

/// Adaptor for a linear system specified by a functor following
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <size_t BlockSize_, typename MathPolicy_, typename ProductF_>
class BlockSystemMatrix
    : public Eigen::EigenBase<
          BlockSystemMatrix<BlockSize_, MathPolicy_, ProductF_>> {
  using self_type = BlockSystemMatrix<BlockSize_, MathPolicy_, ProductF_>;

  using math_policy = MathPolicy_;
  using type_policy = typename math_policy::type_policy;

public:
  using Scalar = typename type_policy::real;
  using RealScalar = typename type_policy::real;
  using StorageIndex = int;

  enum {
    ColsAtCompileTime = Eigen::Dynamic,
    MaxColsAtCompileTime = Eigen::Dynamic,
    IsRowMajor = false
  };

  Eigen::Index outerSize() const {
    return static_cast<Eigen::Index>(_size * BlockSize_);
  }

  Eigen::Index rows() const { return outerSize(); }
  Eigen::Index cols() const { return outerSize(); }

  template <typename Rhs>
  Eigen::Product<self_type, Rhs, Eigen::AliasFreeProduct>
  operator*(const Eigen::MatrixBase<Rhs> &x) const {
    return Eigen::Product<self_type, Rhs, Eigen::AliasFreeProduct>(
        *this, x.derived());
  }

  BlockSystemMatrix(size_t size_, ProductF_ functor_)
      : _size{size_}, _functor{functor_} {}

public:
  size_t system_size() const { return _size; }
  auto const &system_functor() const { return _functor; }

private:
  size_t _size;
  ProductF_ _functor;
};

// }}}

// {{{ make_block_system_matrix(size, functor)

template <size_t BlockSize_, typename MathPolicy_, typename ProductF_>
auto make_block_system_matrix(size_t size, ProductF_ &&functor) {
  return BlockSystemMatrix<
      BlockSize_, MathPolicy_,
      std::remove_const_t<std::remove_reference_t<ProductF_>>>{
      size, std::forward<ProductF_>(functor)};
}

// }}}

// {{{ BlockDiagonalMatrix

template <
    size_t BlockSize_, typename StorageIndex_, typename Scalar_,
    typename DiagonalF_>
class BlockDiagonalMatrix {
  static constexpr size_t N = BlockSize_;

public:
  using StorageIndex = StorageIndex_;
  using Scalar = Scalar_;

  enum {
    ColsAtCompileTime = Eigen::Dynamic,
    MaxColsAtCompileTime = Eigen::Dynamic
  };

  BlockDiagonalMatrix() {}

  void init(size_t size_, DiagonalF_ *functor_) {
    _size = size_;
    _functor = functor_;
    _diagonal.resize(_size);
  }

  Eigen::Index rows() const { return _size * BlockSize_; }
  Eigen::Index cols() const { return _size * BlockSize_; }

  Eigen::ComputationInfo info() { return Eigen::Success; }

  template <typename MatType>
  BlockDiagonalMatrix &analyzePattern(const MatType &) {
    // nothing to do
    return *this;
  }

  template <typename MatType> BlockDiagonalMatrix &factorize(const MatType &) {
    // nothing to do
    return *this;
  }

  template <typename MatType> BlockDiagonalMatrix &compute(const MatType &) {
#pragma omp parallel
    {
#pragma omp for
      for (size_t i = 0; i < _size; i++) {
        _diagonal[i] =
            (*_functor)(i).completeOrthogonalDecomposition().pseudoInverse();
      }
    }
    return *this;
  }

  template <typename RightHandSide_, typename Column_>
  void
  _solve_impl(const RightHandSide_ &right_hand_side, Column_ &column) const {
#pragma omp parallel
    {
#pragma omp for
      for (size_t i = 0; i < _size; i++) {
        column.template block<BlockSize_, 1>(BlockSize_ * i, 0) =
            _diagonal[i] *
            right_hand_side.template block<BlockSize_, 1>(BlockSize_ * i, 0);
      }
    }
  }

  template <typename RightHandSide_>
  inline const Eigen::Solve<BlockDiagonalMatrix, RightHandSide_>
  solve(const Eigen::MatrixBase<RightHandSide_> &b) const {
    return Eigen::Solve<BlockDiagonalMatrix, RightHandSide_>(
        *this, b.derived());
  }

private:
  size_t _size;
  DiagonalF_ *_functor = nullptr;
  std::vector<Eigen::Matrix<Scalar, BlockSize_, BlockSize_>> _diagonal;
};

// }}}

} // namespace n_eigen

template <typename TypePolicy_> struct eigen_math_policy {
private:
  using real = typename TypePolicy_::real;
  using integer = typename TypePolicy_::integer;
  using boolean = typename TypePolicy_::boolean;

public:
  using type_policy = TypePolicy_;

public:
  template <dtype DType_, size_t... Ns_>
  using ndtype_t = n_eigen::select_eigen_dtype_t<TypePolicy_, DType_, Ns_...>;

public:
  template <typename NDType_, size_t Dimension_>
  static constexpr size_t extent_v =
      n_eigen::eigen_extent_v<NDType_, Dimension_>;

public:
  struct operations {
    template <dtype DType_, size_t... Ns_>
    static ndtype_t<DType_, Ns_...>
    narray(core::narray_t<ndtype_t<DType_>, Ns_...> value_) {
      if constexpr (sizeof...(Ns_) == 0)
        return value_;
      else {
        using result_type = ndtype_t<DType_, Ns_...>;
        return result_type{
            Eigen::Map<result_type>{core::narray_data(value_)}.transpose()};
      }
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) dot(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_).dot(std::forward<RHS_>(rhs_));
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) cross(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_).cross(std::forward<RHS_>(rhs_));
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) outer_product(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_) * std::forward<RHS_>(rhs_).transpose();
    }

    template <typename Arg_> static decltype(auto) trace(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).trace();
    }

    template <typename Arg_> static decltype(auto) transpose(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).transpose();
    }

    template <typename Arg_> static decltype(auto) diagonal(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).diagonal();
    }

    template <typename Arg_> static decltype(auto) norm(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).norm();
    }

    template <typename Arg_> static decltype(auto) norm_squared(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).squaredNorm();
    }

    template <typename Arg_> static decltype(auto) normalized(Arg_ &&arg_) {
      // TODO: Eigen-specific: if the norm(arg_) is very small, returns arg_
      return std::forward<Arg_>(arg_).normalized();
    }

    template <typename... Args_> static decltype(auto) max(Args_ &&... args_) {
      // TODO: support max for non-real-non-scalars
      return std::max(
          std::initializer_list<real>{std::forward<Args_>(args_)...});
    }

    template <typename... Args_> static decltype(auto) min(Args_ &&... args_) {
      // TODO: support max for non-real-non-scalars
      return std::min(
          std::initializer_list<real>{std::forward<Args_>(args_)...});
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) cmul(LHS_ &&lhs_, RHS_ &&rhs_) {
      return (std::forward<LHS_>(lhs_).array() *
              std::forward<RHS_>(rhs_).array())
          .matrix();
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) cdiv(LHS_ &&lhs_, RHS_ &&rhs_) {
      return (std::forward<LHS_>(lhs_).array() /
              std::forward<RHS_>(rhs_).array())
          .matrix();
    }

    template <typename Derived_>
    static decltype(auto) cabs(Eigen::ArrayBase<Derived_> arg_) {
      return arg_.derived().abs();
    }

    template <typename Arg_> static decltype(auto) cabs(Arg_ &&arg_) {
      return std::abs(std::forward<Arg_>(arg_));
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) cmax(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_).max(std::forward<RHS_>(rhs_));
    }

    template <typename LHS_, typename RHS_>
    static decltype(auto) cmin(LHS_ &&lhs_, RHS_ &&rhs_) {
      return std::forward<LHS_>(lhs_).max(std::forward<RHS_>(rhs_));
    }

    template <typename Arg_>
    static decltype(auto) maximum_component(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).maxCoeff();
    }

    template <typename Arg_>
    static decltype(auto) minimum_component(Arg_ &&arg_) {
      return std::forward<Arg_>(arg_).minCoeff();
    }

    /// Compute the Penrose-Moore Pseudo-Inverse.
    template <typename A_> static decltype(auto) invert_pm(A_ &&a_) {
      return std::forward<A_>(a_)
          .completeOrthogonalDecomposition()
          .pseudoInverse()
          .eval();
    }

    /// Solve a positive- or negative-semi-definite linear system of equation.
    template <typename A_, typename B_>
    static decltype(auto) solve_sd(A_ &&a_, B_ &&b_) {
      return std::forward<A_>(a_).ldlt().solve(std::forward<B_>(b_)).eval();
    }

    template <typename Arg_> static decltype(auto) smoothstep(Arg_ &&arg) {
      real const x =
          std::min(std::max(std::forward<Arg_>(arg), real{0}), real{1});
      return x * x * (3 - 2 * x);
    }

    // TODO: really neccessary?
    template <typename Arg_, typename Eps_>
    static real reciprocal_or_zero(Arg_ &&arg, Eps_ &&eps) {
      if (real const x = std::forward<Arg_>(arg);
          std::abs(x) > std::forward<Eps_>(eps))
        return 1 / x;
      else
        return real{0};
    }

    /// Unit Step / Heaviside Step function (left-continuous variant).
    ///   x \mapsto
    ///       1   if x > eps
    ///       0   otherwise
    template <typename Eps_, typename... Arg_>
    static real unit_step_l(Eps_ &&eps_, Arg_ &&... arg) {
      real const eps = static_cast<real>(std::forward<Eps_>(eps_));
      if (((static_cast<real>(std::forward<Arg_>(arg)) > eps) or ...))
        return real{1};
      else
        return real{0};
    }

    /// Unit Step / Heaviside Step function (right-continuous variant).
    ///   x \mapsto
    ///       1   if x > eps
    ///       0   otherwise
    template <typename Eps_, typename... Arg_>
    static real unit_step_r(Eps_ &&eps_, Arg_ &&... arg) {
      real const eps = static_cast<real>(std::forward<Eps_>(eps_));
      if (((static_cast<real>(std::forward<Arg_>(arg)) > eps) or ...))
        return real{1};
      else
        return real{0};
    }

    template <typename Arg_>
    static ndtype_t<dtype::real, 3, 3>
    cross_product_matrix_from_vector(Arg_ const &arg) {
      real const zero = 0;
      return narray<dtype::real, 3, 3>({{
          {zero, -arg[2], arg[1]},
          {arg[2], zero, -arg[0]},
          {-arg[1], arg[0], zero},
      }});
    }

    template <typename Arg_>
    static ndtype_t<dtype::real, 3>
    vector_from_cross_product_matrix(Arg_ const &arg) {
      // TODO: assert that arg is a 3x3 matrix
      return {arg(2, 1), arg(0, 2), arg(1, 0)};
    }

    template <dtype DType_, size_t... Ns_> static decltype(auto) zeros() {
      if constexpr (sizeof...(Ns_) >= 1)
        return ndtype_t<DType_, Ns_...>::Zero();
      else
        return static_cast<ndtype_t<DType_>>(0);
    }

    template <dtype DType_, size_t... Ns_> static decltype(auto) ones() {
      if constexpr (sizeof...(Ns_) >= 1)
        return ndtype_t<DType_, Ns_...>::Ones();
      else
        return static_cast<ndtype_t<DType_>>(1);
    }

    template <dtype DType_, size_t... Ns_>
    static decltype(auto) most_negative() {
      static_assert(dtype::real == DType_ or dtype::integer == DType_, "");
      using type = ndtype_t<DType_>;
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<type>::lowest();
      else
        return n_eigen::select_eigen_type_t<type, Ns_...>::Constant(
            most_negative<DType_>());
    }

    template <dtype DType_, size_t... Ns_>
    static decltype(auto) most_positive() {
      static_assert(dtype::real == DType_ or dtype::integer == DType_, "");
      using type = ndtype_t<DType_>;
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<type>::max();
      else
        return n_eigen::select_eigen_type_t<type, Ns_...>::Constant(
            most_positive<DType_>());
    }

    template <dtype DType_, size_t... Ns_>
    static decltype(auto) positive_infinity() {
      static_assert(DType_ == dtype::real, "");
      if constexpr (0 == sizeof...(Ns_))
        return std::numeric_limits<real>::infinity();
      else
        return n_eigen::select_eigen_type_t<real, Ns_...>::Constant(
            positive_infinity<DType_>());
    }

    template <dtype DType_, size_t... Ns_>
    static decltype(auto) negative_infinity() {
      static_assert(DType_ == dtype::real, "");
      if constexpr (0 == sizeof...(Ns_))
        return -std::numeric_limits<real>::infinity();
      else
        return -n_eigen::select_eigen_type_t<real, Ns_...>::Constant(
            positive_infinity<DType_>());
    }

    template <dtype DType_, size_t... Ns_> static decltype(auto) identity() {
      static_assert(DType_ == dtype::real and 2 == sizeof...(Ns_), "");
      return n_eigen::select_eigen_type_t<real, Ns_...>::Identity();
    }

    template <dtype DType_, size_t... Ns_> static decltype(auto) epsilon() {
      static_assert(dtype::real == DType_, "");
      using type = ndtype_t<DType_>;
      if constexpr (0 == sizeof...(Ns_))
        return std::sqrt(std::numeric_limits<type>::epsilon());
      else
        return n_eigen::select_eigen_type_t<type, Ns_...>::Constant(
            epsilon<DType_>());
    }

    template <dtype DType_, typename T_, size_t N_>
    static decltype(auto) from_array(std::array<T_, N_> const &a_) {
      ndtype_t<DType_, N_> result;
      for (size_t n = 0; n < N_; ++n)
        result[static_cast<Eigen::Index>(n)] =
            static_cast<ndtype_t<DType_>>(a_[n]);
      return result;
    }
  };

private:
  struct solve_cg_dp_apply_fn {
    template <typename G_, typename I_, typename Old_, typename New_>
    decltype(auto) operator()(G_, I_, Old_, New_ &&n) const {
      return std::forward<New_>(n);
    }
  };

public:
  // TODO: The interface of this function is extremely messy, hacky and requires
  //       serious refactoring. Possibly into a seperate solver-type.
  template <
      typename NHood_, typename Group_, typename IterateF_, typename ProductF_,
      typename RHSF_, typename DiagonalF_,
      typename ApplyF_ = solve_cg_dp_apply_fn>
  static size_t solve_cg_dp(
      NHood_ const &nhood_, size_t group_count, Group_ &p, IterateF_ iterate,
      ProductF_ product, RHSF_ rhs, DiagonalF_ diagonal, real const tol = 1e-2,
      size_t const max_iterations = 50, ApplyF_ apply = {}) {
    // {{{ implementation
    // {{{
    std::vector<std::vector<std::vector<size_t>>> per_thread_neighbors;

#pragma omp parallel
    {
#pragma omp single
      {
        per_thread_neighbors.resize(static_cast<size_t>(omp_get_num_threads()));
      }

      size_t tid = static_cast<size_t>(omp_get_thread_num());
      auto &neighbors = per_thread_neighbors[tid];
      neighbors.resize(group_count);
    }

    auto _parallel_foreach_particle_in_group = [&](auto &p, auto f) {
      if (p._count == 0)
        return;

#pragma omp parallel
      {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          f(i);
        }
      }
    };

    auto _find_neighbors = [&](auto &p, size_t i) -> auto & {
      size_t tid = static_cast<size_t>(omp_get_thread_num());
      auto &neighbors = per_thread_neighbors[tid];

      // clean up the neighbor storage
      for (auto &pgn : neighbors) {
        pgn.clear();
      }

      // find all neighbors of (p, i)
      nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
        neighbors[n_index].push_back(j);
      });

      return neighbors;
    };
    // }}}

    Eigen::Matrix<real, Eigen::Dynamic, 1> x{p._count};
    Eigen::Matrix<real, Eigen::Dynamic, 1> b{p._count};
    Eigen::Matrix<real, Eigen::Dynamic, 1> guess{p._count};

    _parallel_foreach_particle_in_group(p, [&](size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      auto ii = static_cast<Eigen::Index>(i);
      b(ii) = rhs(p, i, neighbors);
      guess(ii) = iterate(p, i);
    });

    auto system_matrix = n_eigen::make_system_matrix<eigen_math_policy>(
        p._count, [&p, &product, &_find_neighbors](size_t i, auto const &rhs) {
          auto &neighbors = _find_neighbors(p, i);
          return product(p, i, neighbors, rhs);
        });

    auto preconditioner_functor = [&p, &diagonal, &_find_neighbors](size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      return diagonal(p, i, neighbors);
    };

    using SystemMatrixType = decltype(system_matrix);
    using DiagonalMatrixType = n_eigen::DiagonalMatrix<
        SystemMatrixType, decltype(preconditioner_functor)>;

    Eigen::ConjugateGradient<
        SystemMatrixType, Eigen::Lower | Eigen::Upper, DiagonalMatrixType>
        solver;
    solver.preconditioner().init(p._count, &preconditioner_functor);
    solver.setTolerance(tol);
    solver.setMaxIterations(static_cast<Eigen::Index>(max_iterations));
    solver.compute(system_matrix);
    x = solver.solveWithGuess(b, guess);

    if (solver.iterations() > 0) {
      _parallel_foreach_particle_in_group(p, [&](size_t i) {
        auto ii = static_cast<Eigen::Index>(i);
        iterate(p, i) = apply(p, i, iterate(p, i), x(ii));
      });
    }

    return solver.iterations();
    // }}}
  }

public:
  // TODO: The interface of this function is extremely messy, hacky and requires
  //       serious refactoring. Possibly into a seperate solver-type.
  template <
      size_t BlockSize_, typename NHood_, typename Group_, typename IterateF_,
      typename ProductF_, typename RHSF_, typename DiagonalF_, typename GuessF_,
      typename ApplyF_ = solve_cg_dp_apply_fn>
  static size_t solve_block_cg_dp(
      NHood_ const &nhood_, size_t group_count, Group_ &p, IterateF_ iterate,
      ProductF_ product, RHSF_ rhs, DiagonalF_ diagonal, GuessF_ guess,
      real const tol = 1e-2, size_t const max_iterations = 50,
      ApplyF_ apply = {}) {
    // {{{ implementation
    // {{{
    std::vector<std::vector<std::vector<size_t>>> per_thread_neighbors;

#pragma omp parallel
    {
#pragma omp single
      {
        per_thread_neighbors.resize(static_cast<size_t>(omp_get_num_threads()));
      }

      size_t tid = static_cast<size_t>(omp_get_thread_num());
      auto &neighbors = per_thread_neighbors[tid];
      neighbors.resize(group_count);
    }

    auto _parallel_foreach_particle_in_group = [&](auto &p, auto f) {
      if (p._count == 0)
        return;

#pragma omp parallel
      {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          f(i);
        }
      }
    };

    auto _find_neighbors = [&](auto &p, size_t i) -> auto & {
      size_t tid = static_cast<size_t>(omp_get_thread_num());
      auto &neighbors = per_thread_neighbors[tid];

      // clean up the neighbor storage
      for (auto &pgn : neighbors) {
        pgn.clear();
      }

      // find all neighbors of (p, i)
      nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
        neighbors[n_index].push_back(j);
      });

      return neighbors;
    };
    // }}}

    static constexpr size_t N = BlockSize_;

    Eigen::Matrix<real, Eigen::Dynamic, 1> x{p._count * N};
    Eigen::Matrix<real, Eigen::Dynamic, 1> b{p._count * N};
    Eigen::Matrix<real, Eigen::Dynamic, 1> g{p._count * N};

    // copy the right hand side and the guess
    _parallel_foreach_particle_in_group(p, [&](size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      auto ii = static_cast<Eigen::Index>(i);
      b.template block<N, 1>(N * ii, 0) = rhs(p, i, neighbors);
      g.template block<N, 1>(N * ii, 0) = guess(p, i, neighbors);
    });

    auto system_matrix =
        n_eigen::make_block_system_matrix<BlockSize_, eigen_math_policy>(
            p._count,
            [&p, &product, &_find_neighbors](size_t i, auto const &rhs) {
              auto &neighbors = _find_neighbors(p, i);
              return product(p, i, neighbors, rhs);
            });

    auto preconditioner_functor = [&p, &diagonal, &_find_neighbors](size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      return diagonal(p, i, neighbors);
    };

    using SystemMatrixType = decltype(system_matrix);
    using PreconditionerType = n_eigen::BlockDiagonalMatrix<
        BlockSize_, typename SystemMatrixType::StorageIndex,
        typename SystemMatrixType::Scalar, decltype(preconditioner_functor)>;

    Eigen::ConjugateGradient<
        SystemMatrixType, Eigen::Lower | Eigen::Upper, PreconditionerType>
        solver;
    solver.preconditioner().init(p._count, &preconditioner_functor);
    solver.setTolerance(tol);
    solver.setMaxIterations(static_cast<Eigen::Index>(max_iterations));
    solver.compute(system_matrix);
    x = solver.solveWithGuess(b, g);

    _parallel_foreach_particle_in_group(p, [&](size_t i) {
      auto ii = static_cast<Eigen::Index>(i);
      iterate(p, i) = apply(
          p, i, iterate(p, i),
          x.template block<BlockSize_, 1>(BlockSize_ * ii, 0));
    });

    return solver.iterations();
    // }}}
  }
};

} // namespace prtcl::rt

// {{{ implementation details

namespace Eigen {
namespace internal {

/// Implementing the matrix-vector product for the adaptor.
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <typename ModelPolicy_, typename ProductF_, typename Rhs>
struct generic_product_impl<
    typename prtcl::rt::n_eigen::SystemMatrix<ModelPolicy_, ProductF_>, Rhs,
    SparseShape, DenseShape, GemvProduct>
    : generic_product_impl_base<
          prtcl::rt::n_eigen::SystemMatrix<ModelPolicy_, ProductF_>, Rhs,
          generic_product_impl<
              prtcl::rt::n_eigen::SystemMatrix<ModelPolicy_, ProductF_>, Rhs>> {
  using lhs_type = prtcl::rt::n_eigen::SystemMatrix<ModelPolicy_, ProductF_>;

  typedef typename Product<lhs_type, Rhs>::Scalar Scalar;

  template <typename Dest>
  static void scaleAndAddTo(
      Dest &dst, const lhs_type &lhs, const Rhs &rhs, const Scalar &alpha) {
#pragma omp parallel
    {
#pragma omp for
      for (size_t i = 0; i < lhs.system_size(); ++i) {
        auto ii = static_cast<Eigen::Index>(i);
        dst(ii) = alpha * lhs.system_functor()(i, rhs);
      }
    }
  }
};

/// Implementing the matrix-vector product for the adaptor.
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <
    size_t BlockSize_, typename ModelPolicy_, typename ProductF_, typename Rhs>
struct generic_product_impl<
    typename prtcl::rt::n_eigen::BlockSystemMatrix<
        BlockSize_, ModelPolicy_, ProductF_>,
    Rhs, SparseShape, DenseShape, GemvProduct>
    : generic_product_impl_base<
          prtcl::rt::n_eigen::BlockSystemMatrix<
              BlockSize_, ModelPolicy_, ProductF_>,
          Rhs,
          generic_product_impl<
              prtcl::rt::n_eigen::BlockSystemMatrix<
                  BlockSize_, ModelPolicy_, ProductF_>,
              Rhs>> {
  using lhs_type = prtcl::rt::n_eigen::BlockSystemMatrix<
      BlockSize_, ModelPolicy_, ProductF_>;

  typedef typename Product<lhs_type, Rhs>::Scalar Scalar;

  template <typename Dest>
  static void scaleAndAddTo(
      Dest &dst, const lhs_type &lhs, const Rhs &rhs, const Scalar &alpha) {
#pragma omp parallel
    {
#pragma omp for
      for (size_t i = 0; i < lhs.system_size(); ++i) {
        auto ii = static_cast<Eigen::Index>(i);
        dst.template block<BlockSize_, 1>(BlockSize_ * ii, 0) =
            alpha * lhs.system_functor()(i, rhs);
      }
    }
  }
};

} // namespace internal
} // namespace Eigen

// }}}

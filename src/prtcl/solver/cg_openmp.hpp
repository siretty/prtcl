#ifndef PRTCL_SRC_PRTCL_MATH_SOLVER_CG_OPENMP_HPP
#define PRTCL_SRC_PRTCL_MATH_SOLVER_CG_OPENMP_HPP

#include "../math.hpp"

#include <vector>

#include <omp.h>

namespace prtcl {

template <typename T, size_t... N>
class CGOpenMP {
private:
  using ItemType = math::Tensor<T, N...>;
  using ItemVector = std::vector<ItemType>;

public:
  size_t GetSize() const { return _x.size(); }

public:
  void Resize(size_t new_size) {
    _x.resize(new_size);
    _y.resize(new_size);
    _r.resize(new_size);
    _p.resize(new_size);
    _q.resize(new_size);
    _b.resize(new_size);
  }

public:
  template <
      typename Group_, typename SetupB_, typename SetupG_, typename ProductT_,
      typename ProductA_, typename Apply_>
  size_t solve(
      Group_ const &group, SetupB_ rhs, SetupG_ guess, ProductA_ system,
      ProductT_ precond, Apply_ apply, T const tol = 1e-2,
      size_t const max_k = 100) {
    using namespace ::prtcl::math;

    // do nothing if the group is empty
    if (group._count == 0)
      return 0;

    // resize the internal buffers to the group's size
    Resize(group._count);

#pragma omp parallel
    {
      // setup initial iterate (x)
#pragma omp for
      for (size_t i = 0; i < GetSize(); ++i) {
        guess(group, i, _x);
      }

#pragma omp barrier

      // setup initial residual (r = A x - b)
#pragma omp for
      for (size_t i = 0; i < GetSize(); ++i) {
        rhs(group, i, _b);
        system(group, i, _r, _x);
        _r[i] -= _b[i];
      }
    }

    // norm_squared(b)
    T const b_nsq = _dot(_b, _b);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "b_nsq = ", b_nsq);
#endif

    T const thr_r_nsq = max(tol * tol * b_nsq, smallest_positive<T>());

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "thr_r_nsq = ", thr_r_nsq);
#endif

    // setup initial preconditioned iterate
#pragma omp parallel for
    for (size_t i = 0; i < GetSize(); ++i) {
      precond(group, i, _y, _r);
      _p[i] = -_y[i];
    }

    // compute dot(r, r) for iteration 0
    T r_nsq = _dot(_r, _r);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "r_nsq = ", r_nsq);
    core::log::debug("prtcl::rt::openmp", "cg", "y_nsq = ", _dot(_y, _y));
#endif

    // compute dot(r, y) for iteration 0
    T next_r_dot_y = _dot(_r, _y);

    // iteration k
    size_t min_k = 1;
    auto ok = [min_k, max_k, thr_r_nsq](size_t const k, T const cur_r_nsq) {
      if (k < min_k)
        return true;

      if (k > max_k)
        return false;

      if (cur_r_nsq < thr_r_nsq)
        return false;

      return true;
    };

    size_t iter;
    for (iter = 0; ok(iter, r_nsq); ++iter) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      PRTCL_RT_LOG_TRACE_SCOPED("cg iteration");

      core::log::debug("prtcl::rt::openmp", "cg", "k = ", iter);
#endif

      // dot(r_k, y_k)
      T const prev_r_dot_y = next_r_dot_y;

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug(
          "prtcl::rt::openmp", "cg", "dot(r_k, y_k) = ", prev_r_dot_y);
#endif

      // q_k = A p_k
#pragma omp parallel for
      for (size_t i = 0; i < GetSize(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
        PRTCL_RT_LOG_TRACE_SCOPED("cg compute system");
#endif

        system(group, i, _q, _p);
      }

      // dot(p_k, q_k)
      T const p_dot_q = _dot(_p, _q);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug("prtcl::rt::openmp", "cg", "dot(p_k, q_k) = ", p_dot_q);
#endif

      // in case the denominator is too small, break
      if (std::abs(p_dot_q) < tol)
        break;

      // alpha = dot(r_k, y_k) / dot(p_k, A p_k)
      T const alpha = prev_r_dot_y / p_dot_q;

      // x_{k+1} = x_k + alpha p_k
      // r_{k+1} = r_k + alpha q_k
#pragma omp parallel for schedule(static)
      for (size_t i = 0; i < GetSize(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
        PRTCL_RT_LOG_TRACE_SCOPED("cg x_{k+1} r_{k+1}");
#endif

        _x[i] += alpha * _p[i];
        _r[i] += alpha * _q[i];
      }

      // norm_squared(r_{k+1})
      r_nsq = _dot(_r, _r);

      // y_{k+1} = T r_{k+1}
#pragma omp parallel for
      for (size_t i = 0; i < GetSize(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
        PRTCL_RT_LOG_TRACE_SCOPED("cg solve preconditioner");
#endif

        precond(group, i, _y, _r);
      }

      // dot(r_{k+1}, y_{k+1})
      next_r_dot_y = _dot(_r, _y);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug(
          "prtcl::rt::openmp", "cg", "dot(r_{k+1}, y_{k+1}) = ", next_r_dot_y);
#endif

      // in case the denominator is too small, break
      if (std::abs(prev_r_dot_y) < tol)
        break;

      // beta = dot(r_{k+1}, y_{k+1}) / dot(r_k, y_k)
      T const beta = next_r_dot_y / prev_r_dot_y;

      // p_{k+1} = - y_{k+1} + beta p_k
#pragma omp parallel for schedule(static)
      for (size_t i = 0; i < GetSize(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
        PRTCL_RT_LOG_TRACE_SCOPED("cg p_{k+1}");
#endif

        _p[i] = -_y[i] + beta * _p[i];
      }
    }

    // apply the final iterate
#pragma omp parallel for
    for (size_t i = 0; i < GetSize(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      PRTCL_RT_LOG_TRACE_SCOPED("cg apply");
#endif

      apply(group, i, _x);
    }

    return iter;
  }

private:
  T _dot(ItemVector const &lhs, ItemVector const &rhs) const {
    using namespace ::prtcl::math;

    T accumulator = 0;

#pragma omp parallel for schedule(static) reduction(+ : accumulator)
    for (size_t i = 0; i < lhs.size(); ++i) {
#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      PRTCL_RT_LOG_TRACE_SCOPED("cg dot");
#endif

      if constexpr (sizeof...(N) == 0)
        accumulator += lhs[i] * rhs[i];
      else
        accumulator += sum(cmul(lhs[i], rhs[i]));
    }

    return accumulator;
  }

private:
  ItemVector _x, _y, _r, _p, _q, _b;
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_MATH_SOLVER_CG_OPENMP_HPP

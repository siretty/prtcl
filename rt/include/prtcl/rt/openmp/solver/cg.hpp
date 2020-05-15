#pragma once

#include <prtcl/core/log/logger.hpp>

#include <prtcl/rt/common.hpp>
#include <prtcl/rt/type_traits.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

namespace prtcl::rt::openmp {

template <typename ModelPolicy_, size_t... Ns_> class cg {
public:
  using model_policy = ModelPolicy_;

private:
  using m = math_policy_t<model_policy>;
  using o = typename m::operations;

  static constexpr size_t rank = sizeof...(Ns_);

  using real = typename model_policy::type_policy::real;
  using item_type = ndtype_t<m, dtype::real, Ns_...>;
  using vector_type = std::vector<item_type>;

public:
  size_t size() const { return _x.size(); }

public:
  void resize(size_t new_size) {
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
      ProductT_ precond, Apply_ apply, real const tol = 1e-2,
      size_t const max_k = 100) {
    PRTCL_RT_LOG_TRACE_SCOPED("cg solve");
    // do nothing if the group is empty
    if (group._count == 0)
      return 0;

    // resize the internal buffers to the group's size
    resize(group._count);

#pragma omp parallel
    {
      // setup initial iterate (x)
#pragma omp for
      for (size_t i = 0; i < size(); ++i) {
        guess(group, i, _x);
      }

#pragma omp barrier

      // setup initial residual (r = A x - b)
#pragma omp for
      for (size_t i = 0; i < size(); ++i) {
        rhs(group, i, _b);
        system(group, i, _r, _x);
        _r[i] -= _b[i];
      }
    }

    // norm_squared(b)
    real const b_nsq = _dot(_b, _b);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "b_nsq = ", b_nsq);
#endif

    real const thr_r_nsq =
        o::max(tol * tol * b_nsq, o::template smallest_positive<dtype::real>());

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "thr_r_nsq = ", thr_r_nsq);
#endif

    // setup initial preconditioned iterate
#pragma omp parallel for
    for (size_t i = 0; i < size(); ++i) {
      precond(group, i, _y, _r);
      _p[i] = -_y[i];
    }

    // compute dot(r, r) for iteration 0
    real r_nsq = _dot(_r, _r);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
    core::log::debug("prtcl::rt::openmp", "cg", "r_nsq = ", r_nsq);
    core::log::debug("prtcl::rt::openmp", "cg", "y_nsq = ", _dot(_y, _y));
#endif

    // compute dot(r, y) for iteration 0
    real next_r_dot_y = _dot(_r, _y);

    // iteration k
    size_t min_k = 1;
    auto ok = [min_k, max_k, thr_r_nsq](size_t const k, real const cur_r_nsq) {
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
      PRTCL_RT_LOG_TRACE_SCOPED("cg iteration");

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug("prtcl::rt::openmp", "cg", "k = ", iter);
#endif

      // dot(r_k, y_k)
      real const prev_r_dot_y = next_r_dot_y;

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug(
          "prtcl::rt::openmp", "cg", "dot(r_k, y_k) = ", prev_r_dot_y);
#endif

      // q_k = A p_k
#pragma omp parallel for
      for (size_t i = 0; i < size(); ++i) {
        PRTCL_RT_LOG_TRACE_SCOPED("cg compute system");

        system(group, i, _q, _p);
      }

      // dot(p_k, q_k)
      real const p_dot_q = _dot(_p, _q);

#ifdef PRTCL_RT_OPENMP_SOLVER_DEBUG
      core::log::debug("prtcl::rt::openmp", "cg", "dot(p_k, q_k) = ", p_dot_q);
#endif

      // in case the denominator is too small, break
      if (std::abs(p_dot_q) < tol)
        break;

      // alpha = dot(r_k, y_k) / dot(p_k, A p_k)
      real const alpha = prev_r_dot_y / p_dot_q;

      // x_{k+1} = x_k + alpha p_k
      // r_{k+1} = r_k + alpha q_k
#pragma omp parallel for schedule(static)
      for (size_t i = 0; i < size(); ++i) {
        PRTCL_RT_LOG_TRACE_SCOPED("cg x_{k+1} r_{k+1}");

        _x[i] += alpha * _p[i];
        _r[i] += alpha * _q[i];
      }

      // norm_squared(r_{k+1})
      r_nsq = _dot(_r, _r);

      // y_{k+1} = T r_{k+1}
#pragma omp parallel for
      for (size_t i = 0; i < size(); ++i) {
        PRTCL_RT_LOG_TRACE_SCOPED("cg solve preconditioner");

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
      real const beta = next_r_dot_y / prev_r_dot_y;

      // p_{k+1} = - y_{k+1} + beta p_k
#pragma omp parallel for schedule(static)
      for (size_t i = 0; i < size(); ++i) {
        PRTCL_RT_LOG_TRACE_SCOPED("cg p_{k+1}");

        _p[i] = -_y[i] + beta * _p[i];
      }
    }

    // apply the final iterate
#pragma omp parallel for
    for (size_t i = 0; i < size(); ++i) {
      PRTCL_RT_LOG_TRACE_SCOPED("cg apply");

      apply(group, i, _x);
    }

    return iter;
  }

private:
  real _dot(vector_type const &lhs, vector_type const &rhs) const {
    real accumulator = 0;

#pragma omp parallel for schedule(static) reduction(+ : accumulator)
    for (size_t i = 0; i < lhs.size(); ++i) {
      PRTCL_RT_LOG_TRACE_SCOPED("cg dot");

      if constexpr (rank == 0)
        accumulator += lhs[i] * rhs[i];
      else
        accumulator += o::sum(o::cmul(lhs[i], rhs[i]));
    }

    return accumulator;
  }

private:
  vector_type _x, _y, _r, _p, _q, _b;
};

} // namespace prtcl::rt::openmp

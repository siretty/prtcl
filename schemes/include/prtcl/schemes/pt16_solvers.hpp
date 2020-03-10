#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/logger.hpp>
#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

#include <Eigen/Eigen>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl {
namespace schemes {

template <typename, typename> class SystemMatrix;

} // namespace schemes
} // namespace prtcl

namespace Eigen {
namespace internal {

template <typename ModelPolicy_, typename ProductF_>
struct traits<prtcl::schemes::SystemMatrix<ModelPolicy_, ProductF_>>
    : public Eigen::internal::traits<
          Eigen::SparseMatrix<typename ModelPolicy_::type_policy::real>> {};

} // namespace internal
} // namespace Eigen

namespace prtcl {
namespace schemes {

/// Adaptor for a linear system specified by a functor following
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <typename ModelPolicy_, typename ProductF_>
class SystemMatrix
    : public Eigen::EigenBase<SystemMatrix<ModelPolicy_, ProductF_>> {
  using self_type = SystemMatrix<ModelPolicy_, ProductF_>;

  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;

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

template <typename ModelPolicy_, typename ProductF_>
auto make_system_matrix(size_t size, ProductF_ &&functor) {
  return SystemMatrix<
      ModelPolicy_, std::remove_const_t<std::remove_reference_t<ProductF_>>>{
      size, std::forward<ProductF_>(functor)};
}

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

template <typename ModelPolicy_> class pt16_solvers {
public:
  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;
  using data_policy = typename model_policy::data_policy;

  using nd_dtype = prtcl::rt::nd_dtype;

  template <nd_dtype DType_>
  using dtype_t = typename type_policy::template dtype_t<DType_>;
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_t = typename math_policy::template nd_dtype_t<DType_, Ns_...>;
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t =
      typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr size_t N = model_policy::dimensionality;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using group_type = prtcl::rt::basic_group<model_policy>;

private:
  struct global_data {
    nd_dtype_data_ref_t<nd_dtype::real> smoothing_scale;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real>("smoothing_scale");
    }

    void _load(model_type const &m_) {
      smoothing_scale =
          m_.template get_global<nd_dtype::real>("smoothing_scale");
    }
  };

private:
  struct fluid_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // uniform fields
    nd_dtype_data_ref_t<nd_dtype::real> rest_density;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real> mass;
    nd_dtype_data_ref_t<nd_dtype::real, N> vorticity;
    nd_dtype_data_ref_t<nd_dtype::real> pt16_vorticity_diffusion_diagonal;
    nd_dtype_data_ref_t<nd_dtype::real, N> pt16_vorticity_diffusion_rhs;
    nd_dtype_data_ref_t<nd_dtype::real> density;
    nd_dtype_data_ref_t<nd_dtype::real, N, N> target_velocity_gradient;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;
    nd_dtype_data_ref_t<nd_dtype::real> pt16_velocity_reconstruction_diagonal;
    nd_dtype_data_ref_t<nd_dtype::real, N> pt16_velocity_reconstruction_rhs;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<nd_dtype::real>("rest_density");

      // varying fields
      g_.template add_varying<nd_dtype::real>("mass");
      g_.template add_varying<nd_dtype::real, N>("vorticity");
      g_.template add_varying<nd_dtype::real>(
          "pt16_vorticity_diffusion_diagonal");
      g_.template add_varying<nd_dtype::real, N>(
          "pt16_vorticity_diffusion_rhs");
      g_.template add_varying<nd_dtype::real>("density");
      g_.template add_varying<nd_dtype::real, N, N>("target_velocity_gradient");
      g_.template add_varying<nd_dtype::real, N>("velocity");
      g_.template add_varying<nd_dtype::real>(
          "pt16_velocity_reconstruction_diagonal");
      g_.template add_varying<nd_dtype::real, N>(
          "pt16_velocity_reconstruction_rhs");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      rest_density = g_.template get_uniform<nd_dtype::real>("rest_density");

      // varying fields
      mass = g_.template get_varying<nd_dtype::real>("mass");
      vorticity = g_.template get_varying<nd_dtype::real, N>("vorticity");
      pt16_vorticity_diffusion_diagonal =
          g_.template get_varying<nd_dtype::real>(
              "pt16_vorticity_diffusion_diagonal");
      pt16_vorticity_diffusion_rhs = g_.template get_varying<nd_dtype::real, N>(
          "pt16_vorticity_diffusion_rhs");
      density = g_.template get_varying<nd_dtype::real>("density");
      target_velocity_gradient = g_.template get_varying<nd_dtype::real, N, N>(
          "target_velocity_gradient");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
      pt16_velocity_reconstruction_diagonal =
          g_.template get_varying<nd_dtype::real>(
              "pt16_velocity_reconstruction_diagonal");
      pt16_velocity_reconstruction_rhs =
          g_.template get_varying<nd_dtype::real, N>(
              "pt16_velocity_reconstruction_rhs");
      position = g_.template get_varying<nd_dtype::real, N>("position");
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);

    for (auto &group : m_.groups()) {
      if ((group.get_type() == "fluid") and (true)) {
        fluid_data::_require(group);
      }
    }
  }

public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.fluid.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group =
          groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if ((group.get_type() == "fluid") and (true)) {
        auto &data = _data.by_group_type.fluid.emplace_back();
        data._load(group);
        data._index = i;
      }
    }
  }

private:
  struct {
    global_data global;
    struct {
      std::vector<fluid_data> fluid;
    } by_group_type;
  } _data;

  struct per_thread_type {
    std::vector<std::vector<size_t>> neighbors;

    // reductions
  };

  std::vector<per_thread_type> _per_thread;
  size_t _group_count;

  using real = dtype_t<nd_dtype::real>;

private:
  template <
      typename NHood_, typename Group_, typename IterateF_, typename ProductF_,
      typename RHSF_, typename DiagonalF_>
  size_t solve_cg_dp(
      NHood_ const &nhood_, Group_ &p, IterateF_ iterate, ProductF_ product,
      RHSF_ rhs, DiagonalF_ diagonal, real const tol = 1e-2,
      size_t const max_iterations = 50) {
    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    // {{{
    auto _parallel = [this](auto f) {
      _Pragma("omp parallel") {
        _Pragma("omp single") {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread.resize(thread_count);
        } // pragma omp single

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

        // select and resize the neighbor storage for the current thread
        auto &neighbors = _per_thread[thread_index].neighbors;
        neighbors.resize(_group_count);

        for (auto &pgn : neighbors)
          pgn.reserve(100);

        f(thread_index);
      }
    };

    auto _foreach_particle = [this, &nhood_](auto tid, auto &p, auto f) {
#pragma omp for
      for (size_t i = 0; i < p._count; ++i) {
        f(tid, i);
      }
    };

    auto _parallel_foreach_particle_in_group = [&, this](auto &p, auto f) {
      if (p._count == 0)
        return;

      _parallel([&, this](auto tid) {
        _foreach_particle(tid, p, [&, this](auto tid, size_t i) {
          // call the per-particle function
          f(tid, i);
        });
      });
    };

    auto _find_neighbors = [ this, &nhood_ ](auto &p, size_t i) -> auto & {
      size_t tid = static_cast<size_t>(omp_get_thread_num());
      auto &neighbors = _per_thread[tid].neighbors;

      // clean up the neighbor storage
      for (auto &pgn : neighbors)
        pgn.clear();

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

    _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      auto ii = static_cast<Eigen::Index>(i);
      b(ii) = rhs(p, i, neighbors);
      guess(ii) = iterate(p, i);
    });

    auto system_matrix = make_system_matrix<ModelPolicy_>(
        p._count, [&p, &product, &_find_neighbors](size_t i, auto const &rhs) {
          auto &neighbors = _find_neighbors(p, i);
          return product(p, i, neighbors, rhs);
        });

    auto preconditioner_functor = [&p, &diagonal, &_find_neighbors](size_t i) {
      auto &neighbors = _find_neighbors(p, i);
      return diagonal(p, i, neighbors);
    };

    using SystemMatrixType = decltype(system_matrix);
    using DiagonalMatrixType =
        DiagonalMatrix<SystemMatrixType, decltype(preconditioner_functor)>;

    Eigen::ConjugateGradient<
        SystemMatrixType, Eigen::Lower | Eigen::Upper, DiagonalMatrixType>
        solver;
    solver.preconditioner().init(p._count, &preconditioner_functor);
    solver.setTolerance(tol);
    solver.setMaxIterations(static_cast<Eigen::Index>(max_iterations));
    solver.compute(system_matrix);
    x = solver.solveWithGuess(b, guess);

    if (solver.iterations() > 0) {
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
        auto ii = static_cast<Eigen::Index>(i);
        iterate(p, i) = x(ii);
      });
    }

    return solver.iterations();
  }

public:
  template <typename NHood_> size_t vorticity_diffusion(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    size_t d = 0;

    auto diagonal = [](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_diagonal[f];
    };

    auto product = [&g, &diagonal](
                       auto &p, size_t f, auto &neighbors, auto const &x) {
      auto const h = g.smoothing_scale[0];

      real result = diagonal(p, f, neighbors) * x(f);

      for (auto f_f : neighbors[p._index]) {
        if (f != f_f)
          result -= p.mass[f_f] *
                    o::kernel_h(p.position[f] - p.position[f_f], h) * x(f_f);
      }

      return result;
    };

    auto rhs = [&d](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_rhs[f][d];
    };

    auto iterate = [&d](auto &p, size_t f) -> auto & {
      return p.vorticity[f][d];
    };

    size_t iterations = 0;

    for (auto &p : _data.by_group_type.fluid) {
      for (d = 0; d < N; ++d) {
        iterations += solve_cg_dp(nhood_, p, iterate, product, rhs, diagonal);
      }
    }

    return iterations;
  }

public:
  template <typename NHood_>
  size_t velocity_reconstruction(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    size_t d = 0;

    auto diagonal = [&g](auto &p, size_t f, auto &) {
      // return p.pt16_velocity_reconstruction_diagonal[f];
      return p.density[f] -
             p.mass[f] * o::kernel_h(
                             c::template zeros<nd_dtype::real, N>(),
                             g.smoothing_scale[0]);
    };

    auto product = [&g, &diagonal](auto &p, size_t f, auto &neighbors, auto x) {
      auto const h = g.smoothing_scale[0];

      real result = diagonal(p, f, neighbors) * x(f);

      for (auto f_f : neighbors[p._index]) {
        if (f != f_f)
          result -= p.mass[f_f] *
                    o::kernel_h(p.position[f] - p.position[f_f], h) * x(f_f);
      }

      return result;
    };

    auto rhs = [&d](auto &p, size_t f, auto &) {
      return p.pt16_velocity_reconstruction_rhs[f][d];
    };

    auto iterate = [&d](auto &p, size_t f) -> auto & {
      return p.velocity[f][d];
    };

    size_t iterations = 0;

    for (auto &p : _data.by_group_type.fluid) {
      for (d = 0; d < N; ++d) {
        iterations += solve_cg_dp(
            nhood_, p, iterate, product, rhs, diagonal,
            10 * 1e-5 * p.rest_density[0], 500);
      }
    }

    return iterations;
  }
};

} // namespace schemes
} // namespace prtcl

namespace Eigen {
namespace internal {

/// Implementing the matrix-vector product for the adaptor.
/// https://eigen.tuxfamily.org/dox/group__MatrixfreeSolverExample.html
template <typename ModelPolicy_, typename ProductF_, typename Rhs>
struct generic_product_impl<
    typename prtcl::schemes::SystemMatrix<ModelPolicy_, ProductF_>, Rhs,
    SparseShape, DenseShape, GemvProduct>
    : generic_product_impl_base<
          prtcl::schemes::SystemMatrix<ModelPolicy_, ProductF_>, Rhs,
          generic_product_impl<
              prtcl::schemes::SystemMatrix<ModelPolicy_, ProductF_>, Rhs>> {
  using lhs_type = prtcl::schemes::SystemMatrix<ModelPolicy_, ProductF_>;

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
} // namespace internal
} // namespace Eigen

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

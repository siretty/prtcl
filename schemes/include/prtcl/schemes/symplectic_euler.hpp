#pragma once

#include <prtcl/rt/common.hpp>
#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/basic_group.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl::schemes {

template <
  typename ModelPolicy_
>
class symplectic_euler {
public:
  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;
  using data_policy = typename model_policy::data_policy;

  using nd_dtype = prtcl::rt::nd_dtype;

  template <nd_dtype DType_> using dtype_t = typename type_policy::template dtype_t<DType_>;
  template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_t = typename math_policy::template nd_dtype_t<DType_, Ns_...>;
  template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_data_ref_t = typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr size_t N = model_policy::dimensionality;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using group_type = prtcl::rt::basic_group<model_policy>;

private:
  struct global_data {
    nd_dtype_data_ref_t<nd_dtype::real> maximum_speed;
    nd_dtype_data_ref_t<nd_dtype::real> time_step;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real>("maximum_speed");
      m_.template add_global<nd_dtype::real>("time_step");
    }

    void _load(model_type const &m_) {
      maximum_speed = m_.template get_global<nd_dtype::real>("maximum_speed");
      time_step = m_.template get_global<nd_dtype::real>("time_step");
    }
  };

private:
  struct fluid_group_data {
    size_t _count;
    size_t _index;

    // uniform fields

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real, N> acceleration;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;

    static void _require(group_type &g_) {
      // uniform fields

      // varying fields
      g_.template add_varying<nd_dtype::real, N>("acceleration");
      g_.template add_varying<nd_dtype::real, N>("position");
      g_.template add_varying<nd_dtype::real, N>("velocity");
    }

    void _load(group_type const &g_) {
      std::cerr << "loading group " << g_.get_name() << '\n';

      _count = g_.size();

      // uniform fields

      // varying fields
      acceleration = g_.template get_varying<nd_dtype::real, N>("acceleration");
      position = g_.template get_varying<nd_dtype::real, N>("position");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);

    for (auto &group : m_.groups()) {
      if (group.get_type() == "fluid")
        fluid_group_data::_require(group);
    }
  }

public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if (group.get_type() == "fluid") {
        std::cerr << "loading group " << group.get_name() << " with index " << i << '\n';

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
      std::vector<fluid_group_data> fluid;
    } by_group_type;
  } _data;

  struct {
    std::vector<std::vector<std::vector<size_t>>> neighbors;

    // reductions
    std::vector<nd_dtype_t<nd_dtype::real>> rd_maximum_speed;
  } _per_thread;

  size_t _group_count;

  // start of child #0 procedure
public:
  template <typename NHood_>
  void advect_symplectic_euler(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member (types)
    using o = typename math_policy::operations;
    using c = typename math_policy::constants;

    // start of child #0 equation
    g.maximum_speed[0] = c::template negative_infinity<nd_dtype::real>() ;
    // close of child #0 equation

    // start of child #1 foreach_particle
    #pragma omp parallel
    {
      #pragma omp single
      {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());

        _per_thread.rd_maximum_speed.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select the per-thread reduction variables
      auto &rd_maximum_speed = _per_thread.rd_maximum_speed[thread_index];

      // initialize the per-thread reduction variables
      rd_maximum_speed = c::template negative_infinity<nd_dtype::real>();

      // start of child #0 if_group_type
      for (auto &p : _data.by_group_type.fluid) {
        #pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // start of child #0 equation
          p.velocity[i] += ( g.time_step[0] ) * ( p.acceleration[i] ) ;
          // close of child #0 equation

          // start of child #1 equation
          p.position[i] += ( g.time_step[0] ) * ( p.velocity[i] ) ;
          // close of child #1 equation

          // start of child #2 reduction
          rd_maximum_speed = o::max( rd_maximum_speed, o::norm( p.velocity[i] ) ) ;
          // close of child #2 reduction
        }
      }
      // close of child #0 if_group_type

      // combine global reductions
      #pragma omp critical
      {
        g.maximum_speed[0] = o::max( g.maximum_speed[0], rd_maximum_speed ) ;
      } // pragma omp critical
    } // pragma omp parallel
    // close of child #1 foreach_particle
  }
  // close of child #0 procedure
};

} // namespace prtcl::schemes

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

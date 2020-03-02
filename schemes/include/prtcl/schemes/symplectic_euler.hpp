
#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl { namespace schemes {

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
    nd_dtype_data_ref_t<nd_dtype::real> fade_duration;
    nd_dtype_data_ref_t<nd_dtype::real> time_step;
    nd_dtype_data_ref_t<nd_dtype::real> maximum_speed;
    nd_dtype_data_ref_t<nd_dtype::real> current_time;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real>("fade_duration");
      m_.template add_global<nd_dtype::real>("time_step");
      m_.template add_global<nd_dtype::real>("maximum_speed");
      m_.template add_global<nd_dtype::real>("current_time");
    }

    void _load(model_type const &m_) {
      fade_duration = m_.template get_global<nd_dtype::real>("fade_duration");
      time_step = m_.template get_global<nd_dtype::real>("time_step");
      maximum_speed = m_.template get_global<nd_dtype::real>("maximum_speed");
      current_time = m_.template get_global<nd_dtype::real>("current_time");
    }
  };

private:
  struct dynamic_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real, N> acceleration;
    nd_dtype_data_ref_t<nd_dtype::real> time_of_birth;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // varying fields
      g_.template add_varying<nd_dtype::real, N>("acceleration");
      g_.template add_varying<nd_dtype::real>("time_of_birth");
      g_.template add_varying<nd_dtype::real, N>("velocity");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // varying fields
      acceleration = g_.template get_varying<nd_dtype::real, N>("acceleration");
      time_of_birth = g_.template get_varying<nd_dtype::real>("time_of_birth");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
      position = g_.template get_varying<nd_dtype::real, N>("position");
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);
    
    for (auto &group : m_.groups()) {
      if ((true) and (group.has_tag("dynamic"))) {
        dynamic_data::_require(group);
      }
    }
  }
  
public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.dynamic.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if ((true) and (group.has_tag("dynamic"))) {
        auto &data = _data.by_group_type.dynamic.emplace_back();
        data._load(group);
        data._index = i;
      }
    }
  }
  
private:
  struct {
    global_data global;
    struct {
      std::vector<dynamic_data> dynamic;
    } by_group_type;
  } _data;

  struct per_thread_type {
    std::vector<std::vector<size_t>> neighbors;

    // reductions
    nd_dtype_t<nd_dtype::real> rd_maximum_speed;
  };

  std::vector<per_thread_type> _per_thread;
  size_t _group_count;

public:
  template <typename NHood_>
  void advect_symplectic_euler(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    _Pragma("omp parallel") {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

      _Pragma("omp single") {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());
        _per_thread.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select and initialize this threads reduction variable
      auto &rd_maximum_speed = _per_thread[thread_index].rd_maximum_speed;
      rd_maximum_speed = c::template negative_infinity<nd_dtype::real>();

      for (auto &p : _data.by_group_type.dynamic) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // no neighbours neccessary

          p.velocity[i] += (g.time_step[0] * p.acceleration[i] * o::smoothstep(((g.current_time[0] - p.time_of_birth[i]) / g.fade_duration[0])));

          p.position[i] += (g.time_step[0] * p.velocity[i]);

          rd_maximum_speed = o::max(rd_maximum_speed, o::norm(p.velocity[i]));
        }
      }

      _Pragma("omp critical") {
        PRTCL_RT_LOG_TRACE_SCOPED("reduction");

        // combine all reduction variables

        g.maximum_speed[0] = o::max(rd_maximum_speed);
      } // pragma omp critical
    } // pragma omp parallel
  }
};

} /* namespace schemes*/ } /* namespace prtcl*/


#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
  
#pragma once

#include "common.hpp"

#include "basic_group.hpp"
#include "basic_model.hpp"
#include "initialize_particles.hpp"
#include "log/logger.hpp"
#include "nd_data_base.hpp"
#include "virtual_clock.hpp"

#include <prtcl/core/constpow.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <cmath>
#include <cstddef>

#include <boost/range/algorithm/copy.hpp>

#include <boost/math/constants/constants.hpp>

#include <omp.h>

namespace prtcl::rt {

template <typename ModelPolicy_> class basic_source {
public:
  using model_policy = ModelPolicy_;

private:
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using data_policy = typename ModelPolicy_::data_policy;

  using o = typename math_policy::operations;
  using l = typename math_policy::literals;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_t =
      typename data_policy::template nd_dtype_data_t<DType_, Ns_...>;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t =
      typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr auto N = model_policy::dimensionality;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

  using model_type = basic_model<model_policy>;
  using group_type = basic_group<model_policy>;

  using scheduler_type = virtual_scheduler<real>;
  using duration = typename scheduler_type::duration;

  static_assert(2 <= N and N <= 3, "");

public:
  basic_source() = delete;

  basic_source(basic_source const &) = default;
  basic_source &operator=(basic_source const &) = default;

  basic_source(basic_source &&) = default;
  basic_source &operator=(basic_source &&) = default;

  basic_source(
      model_type &model, group_type &target_group, rvec center, rvec velocity,
      real radius, ssize_t remaining)
      : _model{&model}, _target_group{&target_group}, _center{center},
        _velocity{velocity}, _radius{radius}, _remaining{remaining} {
    // compute the (virtual) time between spawns
    auto const h =
        _model->template get_global<nd_dtype::real>("smoothing_scale")[0];
    _regular_spawn_interval = duration{h / o::norm(velocity)};
  }

public:
  auto operator()(virtual_scheduler<real> &scheduler_, duration delay) {
    auto const h =
        _model->template get_global<nd_dtype::real>("smoothing_scale")[0];

    rvec const orientation = o::normalized(_velocity);
    rvec const delta_x = (_regular_spawn_interval + delay).count() * _velocity;

    _position.clear();

    if constexpr (N == 2) {
      throw "not implemented yet";
    }

    // sample from the plane through origin with normal orientation
    if constexpr (N == 3) {
      std::array<rvec, 3> unit_vectors = {
          l::template narray<nd_dtype::real, 3>({{1, 0, 0}}),
          l::template narray<nd_dtype::real, 3>({{0, 1, 0}}),
          l::template narray<nd_dtype::real, 3>({{0, 0, 1}}),
      };
      // choose a unit vector wich is not linearly dependent on orientation
      rvec tmp;
      real const dot_limit = (1 + 1 / std::sqrt(real{3})) / 2;
      for (size_t n = 0; n < unit_vectors.size(); ++n) {
        tmp = unit_vectors[n];
        auto const dot_value = o::dot(orientation, tmp);
        if (std::abs(dot_value) <= dot_limit) {
          break;
        }
      }

      rvec const d1 = o::normalized(o::cross(orientation, tmp));
      rvec const d2 = o::normalized(o::cross(orientation, d1));

      using rvec2 =
          typename math_policy::template nd_dtype_t<nd_dtype::real, 2>;
      using rmat2 =
          typename math_policy::template nd_dtype_t<nd_dtype::real, 2, 2>;

      // get pi as real
      auto const pi = boost::math::constants::pi<real>();
      // compute the spawning twistangle
      auto const twist_step = 2 * pi * (2 * _radius * pi) / h;
      real const twist = _age * twist_step;
      // compute the corresponding rotation matrix
      rmat2 const plane_rotation = l::template narray<nd_dtype::real, 2, 2>(
          {{{std::cos(twist), -std::sin(twist)},
            {std::sin(twist), std::cos(twist)}}});

      int const half_extent = std::floor(_radius / h);
      for (int i1 = -half_extent; i1 <= half_extent; ++i1) {
        for (int i2 = -half_extent; i2 <= half_extent; ++i2) {
          rvec2 const plane_x = plane_rotation * rvec2{i1 * h, i2 * h};
          // sample a regular grid
          rvec local_x = plane_x[0] * d1 + plane_x[1] * d2;
          // filter out any positions not inside the radius
          if (o::norm(local_x) > _radius)
            continue;
          // accept the positions that were not filtered
          _position.push_back(_center + local_x + delta_x);
        }
      }
    }

    // create the new particles
    auto indices = _target_group->create(_position.size());
    initialize_particles(*_model, *_target_group, indices);

    // fetch target group fields
    auto rho0 =
        _target_group->template get_uniform<nd_dtype::real>("rest_density")[0];
    auto x = _target_group->template get_varying<nd_dtype::real, N>("position");
    auto v = _target_group->template get_varying<nd_dtype::real, N>("velocity");
    auto m = _target_group->template get_varying<nd_dtype::real>("mass");
    auto t_b =
        _target_group->template get_varying<nd_dtype::real>("time_of_birth");
    // initialize created particles
    for (size_t i = 0; i < _position.size(); ++i) {
      x[indices[i]] = _position[i];
      v[indices[i]] = _velocity;
      m[indices[i]] = static_cast<real>(prtcl::core::constpow(h, N)) * rho0;
      t_b[indices[i]] = scheduler_.clock().now().time_since_epoch().count();
    }

    // adjust the remaining particle count
    _remaining -= _position.size();
    _position.clear();

    log::debug(
        "app", "source", "created ", indices.size(), " of ", _remaining,
        " particles in group ", _target_group->get_name());

    // increase age
    ++_age;

    // reschedule if this source is not finished
    if (_remaining > 0) {
      auto after = _regular_spawn_interval - delay;
      log::debug("app", "source", "rescheduling after ", after.count(), "s");
      return scheduler_.reschedule_after(after);
    } else {
      log::debug("app", "source", "source exhausted, not rescheduling");
      return scheduler_.do_nothing();
    }
  }

private:
  model_type *_model;
  group_type *_target_group;
  rvec _center, _velocity;
  real _radius;
  ssize_t _remaining;
  std::vector<rvec> _position;
  size_t _age = 0;

  duration _regular_spawn_interval;
};

} // namespace prtcl::rt

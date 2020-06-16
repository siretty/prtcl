#ifndef PRTCL_SRC_PRTCL_UTIL_HCP_LATTICE_SOURCE_HPP
#define PRTCL_SRC_PRTCL_UTIL_HCP_LATTICE_SOURCE_HPP

#include "../cxx.hpp"
#include "../data/group.hpp"
#include "../data/model.hpp"
#include "../errors/not_implemented_error.hpp"
#include "../math.hpp"
#include "constpow.hpp"
#include "scheduler.hpp"

#include <vector>

namespace prtcl {

class HCPLatticeSource {
private:
  static auto SQRT_3() { return static_cast<double>(std::sqrt(3.0L)); }
  static auto SQRT_6() { return static_cast<double>(std::sqrt(6.0L)); }

  using Duration = typename VirtualScheduler::Duration;

public:
  HCPLatticeSource() = delete;

  HCPLatticeSource(
      Model &model, Group &group, double radius,
      DynamicTensorT<double, 1> center, DynamicTensorT<double, 1> velocity,
      cxx::count_t remaining)
      : model_{&model}, group_{&group}, radius_{radius}, center_{center_},
        velocity_{velocity}, remaining_{remaining} {
    auto &global = model_->GetGlobal();

    // fetch the smoothing scale
    if (auto *field = global.TryGetFieldImpl<float>("smoothing_scale"))
      smoothing_scale_ = field->GetAccessImpl().GetItem(0);
    else if (auto *field = global.TryGetFieldImpl<double>("smoothing_scale"))
      smoothing_scale_ = field->GetAccessImpl().GetItem(0);
    else
      throw NotImplementedError{};

    // compute the height of the layers
    double const height = SQRT_6() * smoothing_scale_ / 3;

    // compute the (virtual) time between spawns
    regular_spawn_interval_ = Duration{height / math::norm(velocity)};
  }

public:
  auto operator()(VirtualScheduler &scheduler, Duration delay) {
    // if constexpr (N == 2) {
    //  throw "not implemented yet";
    //}

    using RVec3 = TensorT<double, 3>;

    auto const h = smoothing_scale_;
    auto const g = h * 1.2;

    // compute the direction of the source
    RVec3 const orientation = math::normalized(velocity_);
    // correct for delayed source execution
    RVec3 const delta_x = (regular_spawn_interval_ + delay).count() * velocity_;

    position_.clear();

    // sample from the plane through origin with normal orientation
    // if constexpr (N == 3) {
    std::array<RVec3, 3> unit_vectors = {
        RVec3{1., 0., 0.},
        RVec3{0., 1., 0.},
        RVec3{0., 0., 1.},
    };

    // choose a unit vector which is not linearly dependent on orientation
    RVec3 tmp;
    auto const dot_limit = (1. + 1. / std::sqrt(3.)) / 2.;
    for (size_t n = 0; n < unit_vectors.size(); ++n) {
      tmp = unit_vectors[n];
      auto const dot_value = math::dot(orientation, tmp);
      if (std::abs(dot_value) <= dot_limit) {
        break;
      }
    }

    RVec3 const d1 = math::normalized(math::cross(orientation, tmp));
    RVec3 const d2 = math::normalized(math::cross(orientation, d1));

    using RVec2 = TensorT<double, 2>;

    // column and row offsets for planar (triangular) grid
    RVec2 const coffset{g, 0}, roffset{g / 2, SQRT_3() * g / 2};
    // offset for the current layer for planar (triangular) grid
    RVec2 loffset{0, 0};
    switch (age_ % 2) {
#ifdef PRTCL_RT_HCP_LATTICE_DEBUG
    case 0:
      log::debug("app", "source", "hcp lattice layer a");
      break;
#endif
    case 1:
#ifdef PRTCL_RT_HCP_LATTICE_DEBUG
      log::debug("app", "source", "hcp lattice layer b");
#endif
      loffset = RVec2{g / 2, SQRT_3() * g / 6};
      break;
    }

    int const half_extent = static_cast<int>(std::floor(radius_ / h)) + 1;
    for (int i1 = -half_extent; i1 <= half_extent; ++i1) {
      for (int i2 = -half_extent; i2 <= half_extent; ++i2) {
        // sample a regular planar grid
        RVec2 const plane_x = loffset + i1 * coffset + i2 * roffset;
        RVec3 local_x = plane_x[0] * d1 + plane_x[1] * d2;
        // filter out any positions not inside the radius
        if (math::norm(plane_x) > radius_)
          continue;
        // accept the positions that were not filtered
        position_.push_back(center_ + local_x + delta_x);
      }
    }
    //}

    // create the new particles
    auto indices = group_->CreateItems(position_.size());
    // TODO: initialize_particles(*model_, *group_, indices);

    /* TODO: !!!! IMPLEMENT THIS !!!!
    // fetch target group fields
    auto rho0 =
        _target_group->template get_uniform<dtype::real>("rest_density")[0];
    auto x = _target_group->template get_varying<dtype::real, N>("position");
    auto v = _target_group->template get_varying<dtype::real, N>("velocity");
    auto m = _target_group->template get_varying<dtype::real>("mass");
    auto t_b =
        _target_group->template get_varying<dtype::real>("time_of_birth");

    // initialize created particles
    for (size_t i = 0; i < position_.size(); ++i) {
      using difference_type = typename decltype(indices)::difference_type;
      auto const j = indices[static_cast<difference_type>(i)];
      x[j] = position_[i];
      v[j] = velocity_;
      m[j] = constpow(h, 3) * rho0;
      t_b[j] = scheduler.GetClock().now().time_since_epoch().count();
    }
    */

    // adjust the remaining particle count
    remaining_ -= static_cast<ssize_t>(position_.size());
    position_.clear();

#ifdef PRTCL_RT_HCP_LATTICE_DEBUG
    log::debug(
        "app", "source", "at age ", _age, " created ", indices.size(), " of ",
        _remaining, " particles in group ", _target_group->get_name());
#endif

    // increase age
    ++age_;

    // reschedule if this source is not finished
    if (remaining_ > 0) {
      auto after = regular_spawn_interval_ - delay;
#ifdef PRTCL_RT_HCP_LATTICE_DEBUG
      log::debug("app", "source", "rescheduling after ", after.count(), "s");
#endif
      return scheduler.RescheduleAfter(after);
    } else {
#ifdef PRTCL_RT_HCP_LATTICE_DEBUG
      log::debug("app", "source", "source exhausted, not rescheduling");
#endif
      return scheduler.DoNothing();
    }
  }

private:
  Model *model_;
  Group *group_;
  double smoothing_scale_;
  double radius_;
  DynamicTensorT<double, 1> center_;
  DynamicTensorT<double, 1> velocity_;
  cxx::count_t remaining_ = 0;

  std::vector<DynamicTensorT<double, 1>> position_;
  size_t age_ = 0;

  Duration regular_spawn_interval_;
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_UTIL_HCP_LATTICE_SOURCE_HPP

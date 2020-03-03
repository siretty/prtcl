#pragma once

#include "../common.hpp"

#include <prtcl/core/constpow.hpp>

#include <utility>

#include <cmath>

#include <boost/math/constants/constants.hpp>

namespace prtcl::rt {

struct aat13_math_policy_mixin {
  template <typename BaseMathPolicy_> struct mixin {
  private:
    using real = typename BaseMathPolicy_::type_policy::real;

  public:
    struct operations {
      /// Implements equation (2) in [AAT13].
      static real aat13_cohesion_h(real r, real h_diam) {
        auto const pi = boost::math::constants::pi<real>();
        // in [AAT13] h is the support radius and assumed to be double the rest
        // spacing (which is our h)
        auto const h = 2 * h_diam;
        // the pre-factor of the spline
        auto const factor = 32 / (pi * std::pow(h, 9));
        // return a value according to the different cases
        if (h < 2 * r and r <= h)
          return factor * (core::constpow(h - r, 3) * core::constpow(r, 3));
        if (0 <= r and 2 * r <= h)
          return factor * (2 * core::constpow(h - r, 3) * core::constpow(r, 3) -
                           std::pow(h, 6) / 64);
        return 0;
      }

      /// Implements equation (7) in [AAT13].
      static real aat13_adhesion_h(real r, real h_diam) {
        // in [AAT13] h is the support radius and assumed to be double the rest
        // spacing (which is our h)
        auto const h = 2 * h_diam;
        // the pre-factor of the spline
        auto const factor =
            static_cast<real>(0.007) / std::pow(h, static_cast<real>(3.25));
        // return a value according to the different cases
        if (h < 2 * r and r <= h)
          return factor *
                 std::pow(-4 * core::constpow(r, 2) / h + 6 * r - 2 * h, 0.25);
        return 0;
      }
    };

    struct constants {};
  };
};

} // namespace prtcl::rt

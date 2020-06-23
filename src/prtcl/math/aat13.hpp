#pragma once

#include "../util/constpow.hpp"
#include "constants.hpp"

#include <utility>

#include <cmath>

namespace prtcl::math {

/// Implements equation (2) in [AAT13].
template <typename T>
T aat13_cohesion_h(T r, T h_diam) {
  auto const pi = pi_v<T>;
  // in [AAT13] h is the support radius and assumed to be double the rest
  // spacing (which is our h)
  auto const h = 2 * h_diam;
  auto const h6 = std::pow(h, static_cast<T>(6));
  auto const h9 = std::pow(h, static_cast<T>(9));
  // the pre-factor of the spline
  auto const factor = 32 / (pi * h9);
  // return a value according to the different cases
  if (h < 2 * r and r <= h)
    return factor * (constpow(h - r, 3) * constpow(r, 3));
  if (0 <= r and 2 * r <= h)
    return factor * (2 * constpow(h - r, 3) * constpow(r, 3) - h6 / 64);
  return 0;
}

/// Implements equation (7) in [AAT13].
template <typename T>
T aat13_adhesion_h(T r, T h_diam) {
  // in [AAT13] h is the support radius and assumed to be double the rest
  // spacing (which is our h)
  auto const h = 2 * h_diam;
  // the pre-factor of the spline
  auto const factor = static_cast<T>(0.007) / std::pow(h, static_cast<T>(3.25));
  // return a value according to the different cases
  if (h < 2 * r and r <= h) {
    T const b = -4 * constpow(r, 2) / h + 6 * r - 2 * h;
    T const e = static_cast<T>(0.25);
    // max(0, b) is neccessary since b can be slightly negative due to
    // numerical inaccuracies (and even roots of negative numbers are
    // complex)
    return factor * std::pow(std::max(T{0}, b), e);
  }
  return 0;
}

} // namespace prtcl::math

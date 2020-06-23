#ifndef PRTCL_SRC_PRTCL_MATH_CONSTANTS_HPP
#define PRTCL_SRC_PRTCL_MATH_CONSTANTS_HPP

#include "impl.hpp"

#include <limits>

#include <cmath>

namespace prtcl::math {

// ============================================================================
// Constants
// ============================================================================

/// The number pi. \f$ \pi \f$.
template <typename T>
inline constexpr T pi_v = static_cast<T>(
    3.141592653589793238462643383279502884197169399375105820974944592307816406L);

/// The square root of pi. \f$ \sqrt{\pi} \f$.
template <typename T>
inline constexpr T sqrt_pi_v = static_cast<T>(
    1.772453850905516027298167483341145182797549456122387128213807789852911284L);

/// Pi divided by two. \f$ \frac{ \pi }{ 2 } \f$.
template <typename T>
inline constexpr T pi_2_v = static_cast<T>(
    1.570796326794896619231321691639751442098584699687552910487472296153908203L);

/// Pi divided by four. \f$ \frac{ \pi }{ 4 } \f$.
template <typename T>
inline constexpr T pi_4_v = static_cast<T>(
    0.785398163397448309615660845819875721049292349843776455243736148076954101L);

/// The reciprocal of pi. \f$ \frac{ 1 }{ \pi } \f$.
template <typename T>
inline constexpr T pi_r_v = static_cast<T>(
    0.318309886183790671537767526745028724068919291480912897495334688117793595L);

/// The reciprocal of the square root of pi. \f$ \frac{ 1 }{ \sqrt{\pi} } \f$.
template <typename T>
inline constexpr T sqrt_pi_r_v = static_cast<T>(
    0.564189583547756286948079451560772585844050629328998856844085721710642468L);

// NOTE: All numbers are written out to 73 decimal digits, this is enough for
//       numbers with 237bit significand precision ( log10( 2**238 ) < 72 such
//       as the IEEE 754 type binary256.

template <typename T, size_t... N>
decltype(auto) most_negative() {
  return Constant<T, N...>(std::numeric_limits<T>::lowest());
}

template <typename T, size_t... N>
decltype(auto) most_positive() {
  return Constant<T, N...>(std::numeric_limits<T>::max());
}

template <typename T, size_t... N>
decltype(auto) smallest_positive() {
  return Constant<T, N...>(std::numeric_limits<T>::min());
}

template <typename T, size_t... N>
decltype(auto) positive_infinity() {
  return Constant<T, N...>(std::numeric_limits<T>::infinity());
}

template <typename T, size_t... N>
decltype(auto) negative_infinity() {
  return Constant<T, N...>(-std::numeric_limits<T>::infinity());
}

template <typename T, size_t... N>
decltype(auto) epsilon() {
  return Constant<T, N...>(std::sqrt(std::numeric_limits<T>::epsilon()));
}

} // namespace prtcl::math

#endif // PRTCL_SRC_PRTCL_MATH_CONSTANTS_HPP

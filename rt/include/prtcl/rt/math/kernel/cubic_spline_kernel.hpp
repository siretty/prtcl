#pragma once

#include <prtcl/core/constants.hpp>
#include <prtcl/core/constpow.hpp>

#include "kernel_facade.hpp"

namespace prtcl::rt {

template <typename> struct cubic_spline_kernel;

template <typename MathPolicy_>
struct kernel_traits<cubic_spline_kernel<MathPolicy_>> {
  using math_policy = MathPolicy_;
  using type_policy = typename math_policy::type_policy;
};

/// The cubic spline kernel with support radius 2 (see \cite Monaghan1992).
template <typename MathPolicy_>
struct cubic_spline_kernel : kernel_facade<cubic_spline_kernel<MathPolicy_>> {
public:
  using traits_type = kernel_traits<cubic_spline_kernel>;
  using type_policy = typename traits_type::type_policy;
  using math_policy = typename traits_type::math_policy;

  using real = typename type_policy::real;

private:
  friend struct kernel_access;

public:
  using kernel_facade<cubic_spline_kernel>::kernel_facade;

  /// The name of the kernel.
  constexpr static std::string_view get_name() { return "Cubic Spline"; }

private:
  /// The radius of the convex hull of the normalized kernel support.
  constexpr static real get_normalized_support_radius() {
    return static_cast<real>(2);
  }

  /// Evaluate the normalized kernel itself.
  constexpr static real evalq(real q, size_t d) {
    /// Precondition: \f$ q \f$ must be positive
    // SPHXX_ASSERT(q >= 0);

    real result = 0;
    if (q < 1)
      result -= 4 * core::constpow(1 - q, 3);
    if (q < 2)
      result += core::constpow(2 - q, 3);
    return normalization(d) * result;
  }

  /// Evaluate the first normalized kernel derivative.
  constexpr static real evaldq(real q, size_t d) {
    /// Precondition: q must be positive
    // SPHXX_ASSERT(q >= 0);

    /// \f[
    ///   W_\circ'(q) =
    ///   \alpha \left\{ \begin{array}{ll}
    ///     12 (q - 1)^2               & 0 \leq q \leq 1 \\
    ///     12 (q - 1)^2 - 3 (q - 2)^2 & 1 \leq q \leq 2 \\
    ///     0                          & 2 \leq q
    ///   \end{array} \right.
    /// \f]
    real result = 0;
    if (q < 1)
      result += 12 * core::constpow(1 - q, 2);
    if (q < 2)
      result -= 3 * core::constpow(2 - q, 2);
    return normalization(d) * result;
  }

private:
  /// Calculate the normalization factor for this kernel.
  /// This depends on the dimensionality of the kernel (N).
  constexpr static real normalization(size_t d) {
    /// \f[
    ///   \alpha =
    ///   \left\{ \begin{array}{ll}
    ///     \frac{ 1 }{ 6 }                & 1 = N \\
    ///     \frac{ 5 }{ 14 } \frac{1}{\pi} & 2 = N \\
    ///     \frac{ 1 }{ 4 }  \frac{1}{\pi}& 3 = N
    ///   \end{array} \right.
    /// \f]
    switch (d) {
    case 1:
      return static_cast<real>(1.L / 6.L);
    case 2:
      return static_cast<real>(5.L / 14.L * core::pi_r_v<long double>);
    case 3:
      return static_cast<real>(1.L / 4.L * core::pi_r_v<long double>);
    default:
      throw std::runtime_error{"kernel normalization not implemented"};
    };
  }
};

} // namespace prtcl::rt

// References:
// - [Monaghan1992] J. Monaghan, Smoothed Particle Hydrodynamics, "Annual
//   Review of Astronomy and Astrophysics", 30 (1992), pp. 543-574.

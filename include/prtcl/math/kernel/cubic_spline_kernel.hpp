#pragma once

#include "../constants.hpp"
#include "../constpow.hpp"
#include "kernel_facade.hpp"

namespace prtcl {

template <typename> struct cubic_spline_kernel;

template <typename MathTraits>
struct kernel_traits<cubic_spline_kernel<MathTraits>> {
  using math_traits = MathTraits;
};

/// The cubic spline kernel with support radius 2 (see \cite Monaghan1992).
template <typename MathTraits>
struct cubic_spline_kernel : kernel_facade<cubic_spline_kernel<MathTraits>> {
public:
  using traits_type = kernel_traits<cubic_spline_kernel>;
  using math_traits = MathTraits;

  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;
  constexpr static size_t vector_extent = math_traits::vector_extent;

private:
  static_assert(vector_extent >= 1 && vector_extent <= 3);

  friend struct kernel_access;

public:
  using kernel_facade<cubic_spline_kernel>::kernel_facade;

  /// The name of the kernel.
  constexpr static std::string_view get_name() { return "Cubic Spline"; }

private:
  /// The radius of the convex hull of the normalized kernel support.
  constexpr static scalar_type get_normalized_support_radius() {
    return static_cast<scalar_type>(2);
  }

  /// Evaluate the normalized kernel itself.
  constexpr static scalar_type evalq(scalar_type q) {
    /// Precondition: \f$ q \f$ must be positive
    // SPHXX_ASSERT(q >= 0);

    scalar_type result = 0;
    if (q < 1)
      result -= 4 * constpow(1 - q, 3);
    if (q < 2)
      result += constpow(2 - q, 3);
    return normalization() * result;
  }

  /// Evaluate the first normalized kernel derivative.
  constexpr static scalar_type evaldq(scalar_type q) {
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
    scalar_type result = 0;
    if (q < 1)
      result += 12 * constpow(1 - q, 2);
    if (q < 2)
      result -= 3 * constpow(2 - q, 2);
    return normalization() * result;
  }

private:
  /// Calculate the normalization factor for this kernel.
  /// This depends on the dimensionality of the kernel (N).
  constexpr static scalar_type normalization() {
    /// \f[
    ///   \alpha =
    ///   \left\{ \begin{array}{ll}
    ///     \frac{ 1 }{ 6 }                & 1 = N \\
    ///     \frac{ 5 }{ 14 } \frac{1}{\pi} & 2 = N \\
    ///     \frac{ 1 }{ 4 }  \frac{1}{\pi}& 3 = N
    ///   \end{array} \right.
    /// \f]
    if constexpr (1 == vector_extent)
      return static_cast<scalar_type>(1.L / 6.L);
    else if constexpr (2 == vector_extent)
      return static_cast<scalar_type>(5.L / 14.L * pi_r_v<long double>);
    else if constexpr (3 == vector_extent)
      return static_cast<scalar_type>(1.L / 4.L * pi_r_v<long double>);
  }
};

} // namespace prtcl

// References:
// - [Monaghan1992] J. Monaghan, Smoothed Particle Hydrodynamics, "Annual
//   Review of Astronomy and Astrophysics", 30 (1992), pp. 543-574.

#pragma once

#include "../constpow.hpp"
#include "kernel_traits.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

#include <string_view>

namespace prtcl {

// class kernel_access {{{

struct kernel_access {
  template <typename K, typename... Args>
  static auto get_normalized_support_radius(K const &k) {
    return k.get_normalized_support_radius();
  }

  template <typename K, typename... Args>
  static auto evalq(K const &k, Args &&... args) {
    return k.evalq(std::forward<Args>(args)...);
  }

  template <typename K, typename... Args>
  static auto evaldq(K const &k, Args &&... args) {
    return k.evaldq(std::forward<Args>(args)...);
  }
};

// }}}

template <typename Kernel> struct kernel_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(std::declval<kernel_eval>()(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return Kernel{}.eval(std::forward<Args>(args)...);
  }
};

template <typename Kernel> struct kernel_evalgrad {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type =
        decltype(std::declval<kernel_evalgrad>()(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return Kernel{}.evalgrad(std::forward<Args>(args)...);
  }
};

/// Extends the interface of a normalized kernel (in terms of the perfect
/// sampling distance \f$ q \f$) to computations on distances \f$ r \f$ and
/// vectors \f$ v \f$.
template <typename K> struct kernel_facade {
private:
  using impl_traits_type = kernel_traits<K>;
  using math_traits = typename impl_traits_type::math_traits;

public:
  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;

  constexpr static size_t vector_extent = math_traits::vector_extent;

public:
  /// The radius \f$ s \f$ of the kernel support (a ball).
  constexpr scalar_type get_support_radius(scalar_type h) const {
    return h * kernel_access::get_normalized_support_radius(impl());
  }

public:
  /// Evaluate the kernel itself.
  constexpr scalar_type evalr(scalar_type const r, scalar_type h) const {
    return kernel_access::evalq(impl(), std::abs(r) / h) /
           constpow(h, vector_extent);
  }

  /// Evaluate the first kernel derivative.
  constexpr scalar_type evaldr(scalar_type const r, scalar_type h) const {
    return -std::copysign(kernel_access::evaldq(impl(), std::abs(r) / h) /
                              constpow(h, vector_extent + 1),
                          r);
  }

  /// Evaluate the kernel itself.
  template <typename V>
  constexpr scalar_type eval(V const &v, scalar_type h) const {
    return evalr(math_traits::norm(v), h);
  }

  /// Evaluate the kernel gradient.
  constexpr vector_type evalgrad(const vector_type &v, scalar_type h) const {
    scalar_type const r = math_traits::norm(v);
    if (r < math_traits::epsilon())
      return math_traits::make_zero_vector();
    return (evaldr(r, h) / r) * v;
  }

private:
  const K &impl() const { return *static_cast<const K *>(this); }
};

} // namespace prtcl

#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/core/constpow.hpp>

#include "kernel_traits.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

#include <string_view>

namespace prtcl::rt {

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
  using type_policy = typename impl_traits_type::type_policy;
  using math_policy = typename impl_traits_type::math_policy;

public:
  using real = typename type_policy::real;

public:
  /// The radius \f$ s \f$ of the kernel support (a ball).
  constexpr real get_support_radius(real h) const {
    return h * kernel_access::get_normalized_support_radius(impl());
  }

public:
  /// Evaluate the kernel itself.
  constexpr real evalr(real const r, real h, size_t d) const {
    return kernel_access::evalq(impl(), std::abs(r) / h, d) /
           core::constpow(h, d);
  }

  /// Evaluate the first kernel derivative.
  constexpr real evaldr(real const r, real h, size_t d) const {
    return -std::copysign(
        kernel_access::evaldq(impl(), std::abs(r) / h, d) /
            core::constpow(h, d + 1),
        r);
  }

  /// Evaluate the kernel itself.
  template <typename V> constexpr real eval(V const &v, real h) const {
    constexpr size_t d = math_policy::template extent_v<V, 0>;
    return evalr(math_policy::operations::norm(v), h, d);
  }

  /// Evaluate the kernel gradient.
  template <typename V> constexpr auto evalgrad(const V &v, real h) const {
    constexpr size_t d = math_policy::template extent_v<V, 0>;
    using vector_type =
        typename math_policy::template nd_dtype_t<nd_dtype::real, d>;
    real const r = math_policy::operations::norm(v);
    if (r < math_policy::constants::template epsilon<nd_dtype::real>())
      return vector_type{
          math_policy::constants::template zeros<nd_dtype::real, d>()};
    return vector_type{(evaldr(r, h, d) / r) * v};
  }

private:
  const K &impl() const { return *static_cast<const K *>(this); }
};

} // namespace prtcl::rt

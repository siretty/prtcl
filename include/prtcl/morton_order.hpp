#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>

#include <type_traits>

namespace prtcl {

namespace details {

/// Turns signed into unsigned integers while retaining ordering relationships.
template <typename IndexT>
constexpr auto unsign(IndexT);

} // namespace details

/// Compares n-dimensional indices using their z-curve indices without computing
/// the z-curve indices themselves.
struct morton_order_fn {
private:
  template <typename T>
  using size_type_t = decltype(std::declval<T>().size());

public:
  /// Binary comparison function of two vectors of indices (similar to
  /// `std::less<void>`).
  template <typename IndexVecT>
  constexpr bool operator()(IndexVecT const &lhs, IndexVecT const &rhs) const {
    auto result = compare(lhs, rhs);
    assert(result >= 0 || compare(rhs, lhs) > 0);
    assert(result <= 0 || compare(rhs, lhs) < 0);
    assert(result < 0 || result > 0 || compare(rhs, lhs) == 0);
    return result < 0;
  }

  /// Three-way comparison function for vectors of indices.
  template <typename IndexRangeT>
  constexpr int compare(const IndexRangeT &lhs, const IndexRangeT &rhs) const {
    assert(lhs.size() == rhs.size());
    using size_type = size_type_t<IndexRangeT>;
    // initialize the most significant dimension to zero
    size_type msd = 0;
    for (size_type dim = 1; dim < lhs.size(); ++dim) {
      // the XOR eliminates equal bits in the corresponding components in lhs
      // and rhs and checks if the current dimension `dim` is more significant
      // by comparing the most significant differing bits
      if (less_msb(
              unsign(lhs[msd]) ^ unsign(rhs[msd]),
              unsign(lhs[dim]) ^ unsign(rhs[dim])))
        msd = dim;
    }
    // finally compare the indices in the most significant dimension
    auto lhs_msd = unsign(lhs[msd]), rhs_msd = unsign(rhs[msd]);
    return (lhs_msd < rhs_msd) ? -1 : (lhs_msd > rhs_msd) ? 1 : 0;
  }

private:
  /// Checks if the most significant bit in \p lhs is smaller than in \p rhs.
  template <typename T>
  constexpr bool less_msb(T lhs, T rhs) const {
    return (lhs < rhs) && (lhs < (lhs ^ rhs));
  }

  // {{{ implementation details

  /// Turns \p index into an unsigned value such that \f$ a < b \Leftrightarrow
  /// \mathop{\text{unsign}}(a) < \mathop{\text{unsign}}(b) \f$.
  template <typename IndexT>
  constexpr auto unsign(IndexT index) const {
    if constexpr (std::is_signed<IndexT>{}()) {
      using unsigned_type = std::make_unsigned_t<IndexT>;
      constexpr auto signed_max = std::numeric_limits<IndexT>::max();
      if (index < 0)
        return static_cast<unsigned_type>(index + signed_max);
      else
        return static_cast<unsigned_type>(index) +
               static_cast<unsigned_type>(signed_max);
    } else
      return index;
  }

  constexpr static void unsign_sanity_check() {
    morton_order_fn o;
    static_assert(o.unsign<int>(-1) < o.unsign<int>(0));
    static_assert(o.unsign<int>(0) < o.unsign<int>(1));
  }

  // }}}

  // Algorithm: https://en.wikipedia.org/wiki/Z-order_curve
};

constexpr inline morton_order_fn morton_order;

} // namespace prtcl

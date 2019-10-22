#pragma once

#include <array>
#include <numeric>

#include <cstddef>

namespace prtcl {

struct spatial_hash {
  template <typename C> std::size_t operator()(C &&c) const {
    constexpr std::array<std::size_t, 3> factors = {73856093, 19349663,
                                                    83492791};
    return std::inner_product(std::forward<C>(c).begin(),
                              std::forward<C>(c).end(), factors.begin(), 0,
                              std::multiplies<void>{}, std::bit_xor<void>{});
  }
};

} // namespace prtcl

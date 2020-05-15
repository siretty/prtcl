#pragma once

#include <cstddef>

namespace prtcl::rt {

template <
    typename TypePolicy_, template <typename> typename MathPolicy_,
    template <typename> typename DataPolicy_, size_t Dimensionality_>
struct basic_model_policy {
  using type_policy = TypePolicy_;
  using math_policy = MathPolicy_<type_policy>;
  using data_policy = DataPolicy_<math_policy>;
  static constexpr size_t dimensionality = Dimensionality_;
};

template <typename ModelPolicy_>
using type_policy_t = typename ModelPolicy_::type_policy;

template <typename ModelPolicy_>
using math_policy_t = typename ModelPolicy_::math_policy;

template <typename ModelPolicy_>
using data_policy_t = typename ModelPolicy_::data_policy;

template <typename ModelPolicy_>
constexpr size_t dimensionality_v = ModelPolicy_::dimensionality;

} // namespace prtcl::rt

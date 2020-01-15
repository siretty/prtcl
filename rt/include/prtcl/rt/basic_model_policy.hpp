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

} // namespace prtcl::rt

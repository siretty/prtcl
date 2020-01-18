#pragma once

#include <prtcl/rt/common.hpp>

namespace prtcl::rt {

template <template <typename> typename MathPolicy_, typename... Mixins_>
struct mixed_math_policy {
  template <typename TypePolicy_>
  struct policy : public MathPolicy_<TypePolicy_> {
  public:
    using type_policy = TypePolicy_;

  private:
    using base_math_policy = MathPolicy_<TypePolicy_>;

  public:
    template <nd_dtype DType_>
    using dtype_t = typename type_policy::template dtype_t<DType_>;

    template <nd_dtype DType_, size_t... Ns_>
    using nd_dtype_t =
        typename base_math_policy::template nd_dtype_t<DType_, Ns_...>;

    template <typename NDType_, size_t Dimension_>
    static constexpr size_t extent_v =
        base_math_policy::template extent_v<NDType_, Dimension_>;

    struct operations
        : base_math_policy::operations,
          Mixins_::template mixin<base_math_policy>::operations... {};

    struct constants : base_math_policy::constants,
                       Mixins_::template mixin<base_math_policy>::constants... {
    };
  };
};

} // namespace prtcl::rt

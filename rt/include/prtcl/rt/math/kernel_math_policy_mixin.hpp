#pragma once

#include <utility>

namespace prtcl::rt {

template <template <typename> typename Kernel_>
struct kernel_math_policy_mixin {
  template <typename BaseMathPolicy_> struct mixin {
    using kernel_type = Kernel_<BaseMathPolicy_>;

    struct operations {
      template <typename... Args_>
      static decltype(auto) kernel_support_radius(Args_ &&... args_) {
        return kernel_type{}.get_support_radius(std::forward<Args_>(args_)...);
      }

      template <typename... Args_>
      static decltype(auto) kernel_h(Args_ &&... args_) {
        return kernel_type{}.eval(std::forward<Args_>(args_)...);
      }

      template <typename... Args_>
      static decltype(auto) kernel_gradient_h(Args_ &&... args_) {
        return kernel_type{}.evalgrad(std::forward<Args_>(args_)...);
      }
    };

    struct constants {};
  };
};

} // namespace prtcl::rt

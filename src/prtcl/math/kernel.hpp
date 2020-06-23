#pragma once

#include <utility>

namespace prtcl::math {

template <typename Kernel, typename... Args_>
decltype(auto) kernel_support_radius(Args_ &&... args_) {
  return Kernel{}.get_support_radius(std::forward<Args_>(args_)...);
}

template <typename Kernel, typename... Args_>
decltype(auto) kernel_h(Args_ &&... args_) {
  return Kernel{}.eval(std::forward<Args_>(args_)...);
}

template <typename Kernel, typename... Args_>
decltype(auto) kernel_gradient_h(Args_ &&... args_) {
  return Kernel{}.evalgrad(std::forward<Args_>(args_)...);
}

} // namespace prtcl::math

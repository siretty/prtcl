#pragma once

#include <utility>

namespace prtcl::core {

struct identity_fn {
  template <typename Arg_> decltype(auto) operator()(Arg_ &&arg) const {
    return std::forward<Arg_>(arg);
  }
};

constexpr inline identity_fn identity;

} // namespace prtcl::core

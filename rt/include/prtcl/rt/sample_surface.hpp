#pragma once

#include <vector>

namespace prtcl::rt {

template <typename ModelPolicy_, typename OutputIt_>
void sample_mesh_surface(OutputIt_ it_) {
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;

  //
}

} // namespace prtcl::rt

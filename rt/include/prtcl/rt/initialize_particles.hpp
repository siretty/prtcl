#pragma once

#include "basic_group.hpp"
#include "basic_model.hpp"

namespace prtcl::rt {

template <typename ModelPolicy_, typename IndexRange_>
void initialize_particles(
    basic_model<ModelPolicy_> const &model, basic_group<ModelPolicy_> &group,
    IndexRange_ indices) {
  //
}

} // namespace prtcl::rt

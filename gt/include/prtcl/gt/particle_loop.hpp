#pragma once

#include <ostream>

namespace prtcl::gt {

struct particle_loop {
  friend std::ostream &operator<<(std::ostream &o_, particle_loop) {
    return o_ << "particle_loop{}";
  }
};

} // namespace prtcl::gt

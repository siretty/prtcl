#pragma once

#include <ostream>

namespace prtcl::gt {

struct neighbour_loop {
  friend std::ostream &operator<<(std::ostream &o_, neighbour_loop) {
    return o_ << "neighbour_loop{}";
  }
};

} // namespace prtcl::gt

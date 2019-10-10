#pragma once

#include <ostream>

namespace prtcl::tags {

struct sycl {
  friend std::ostream &operator<<(std::ostream &s, sycl) { return s << "sycl"; }
};

} // namespace prtcl::tags

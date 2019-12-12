#pragma once

#include <string>

namespace prtcl::data {

struct group_base {
  virtual ~group_base() {}

  virtual bool has_flag(std::string) const = 0;
};

} // namespace prtcl::data

#pragma once

#include <string>

namespace prtcl::data {

struct group_base {
  virtual ~group_base() {}

  virtual std::string get_type() const = 0;
};

} // namespace prtcl::data

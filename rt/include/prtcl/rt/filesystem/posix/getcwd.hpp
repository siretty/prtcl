#pragma once

#include <string>

#include <cstdlib>

#include <unistd.h>

namespace prtcl::rt::filesystem {

inline std::string getcwd() {
  char *buf = ::getcwd(nullptr, 0);
  auto result = std::string{buf};
  ::free(buf);
  return result;
}

} // namespace prtcl::rt::filesystem

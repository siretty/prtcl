#include <prtcl/schemes/getcwd.hpp>

#ifdef _POSIX_C_SOURCE

#include <cstdlib>

#include <unistd.h>

namespace prtcl::schemes {


std::string getcwd() {
  char * buf = ::getcwd(nullptr, 0);
  auto result = std::string{buf};
  ::free(buf);
  return result;
}

} //

#else
#error "no implementation for getcwd available"
#endif

#pragma once

#include <string>

namespace prtcl::rt::filesystem {

inline std::string getcwd();

} // namespace prtcl::rt::filesystem

// operating-system specific implementation

#if defined(_POSIX_C_SOURCE)
#include "posix/getcwd.hpp"
#endif // defined(_POSIX_C_SOURCE)

#pragma once

#include "../data/group.hpp"

#include <iosfwd>

namespace prtcl {

void SaveVTK(std::ostream &o_, Group const &group);

} // namespace prtcl

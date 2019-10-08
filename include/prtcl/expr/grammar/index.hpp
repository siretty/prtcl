#pragma once

#include "../terminal/active_index.hpp"
#include "../terminal/neighbour_index.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

using AnyIndexTerm = boost::proto::or_<ActiveIndexTerm, NeighbourIndexTerm>;

} // namespace prtcl::expr

#pragma once

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct neighbour_index {};

using neighbour_index_term =
    typename boost::proto::terminal<neighbour_index>::type;

using NeighbourIndexTerm = neighbour_index_term;

} // namespace prtcl::expr

#pragma once

#include "../prtcl_domain.hpp"

#include <ostream>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct neighbour_index {
  friend std::ostream &operator<<(std::ostream &s, neighbour_index const &) {
    return s << "neighbour_index";
  }
};

using neighbour_index_term =
    prtcl_expr<typename boost::proto::terminal<neighbour_index>::type>;

using NeighbourIndexTerm = neighbour_index_term;

} // namespace prtcl::expr

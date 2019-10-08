#pragma once

#include "../prtcl_domain.hpp"

#include <ostream>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct active_index {
  friend std::ostream &operator <<(std::ostream &s, active_index const &) {
    return s << "active_index";
  }
};

using active_index_term = prtcl_expr<typename boost::proto::terminal<active_index>::type>;

using ActiveIndexTerm = active_index_term;

} // namespace prtcl::expr

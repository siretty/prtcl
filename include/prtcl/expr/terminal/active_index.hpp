#pragma once

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct active_index {};

using active_index_term = typename boost::proto::terminal<active_index>::type;

using ActiveIndexTerm = active_index_term;

} // namespace prtcl::expr

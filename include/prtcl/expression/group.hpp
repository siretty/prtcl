#pragma once

#include "../tags.hpp"
#include "expr.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

using active_group = expr<typename boost::proto::terminal<tag::active>::type>;
using passive_group = expr<typename boost::proto::terminal<tag::passive>::type>;

struct Group
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<tag::is_group_tag<boost::proto::_value>()>> {};

} // namespace expression
} // namespace prtcl

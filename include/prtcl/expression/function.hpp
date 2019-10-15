#pragma once

#include "../tags.hpp"
#include "expr.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

using dot_fun = expr<typename boost::proto::terminal<tag::dot>::type>;

using norm_fun = expr<typename boost::proto::terminal<tag::norm>::type>;

using norm_squared_fun =
    expr<typename boost::proto::terminal<tag::norm_squared>::type>;

using normalized_fun =
    expr<typename boost::proto::terminal<tag::normalized>::type>;

using max_fun = expr<typename boost::proto::terminal<tag::max>::type>;

using min_fun = expr<typename boost::proto::terminal<tag::min>::type>;

using kernel_fun = expr<typename boost::proto::terminal<tag::kernel>::type>;

using kernel_gradient_fun =
    expr<typename boost::proto::terminal<tag::kernel_gradient>::type>;

struct Function
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<tag::is_function_tag<boost::proto::_value>()>> {};

} // namespace expression
} // namespace prtcl

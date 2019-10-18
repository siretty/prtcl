#pragma once

#include "../tags.hpp"

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename FunctionTag> struct call {
  static_assert(tag::is_function_tag_v<FunctionTag>, "FunctionTag is invalid");

  using function_tag = FunctionTag;
};

template <typename FT>
using call_term = boost::yap::terminal<boost::yap::expression, call<FT>>;

} // namespace prtcl::expr

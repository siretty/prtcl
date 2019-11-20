#pragma once

#include "../tags.hpp"
#include <prtcl/meta/remove_cvref.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename Select> struct loop {
  // TODO: assert that Select is callable and returns booleans if possible

  Select select;
};

template <typename S>
using loop_term = boost::yap::terminal<boost::yap::expression, loop<S>>;

template <typename Select> auto make_loop(Select &&select_) {
  return loop_term<meta::remove_cvref_t<Select>>{
      {std::forward<Select>(select_)}};
}

} // namespace prtcl::expr

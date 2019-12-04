#pragma once

#include <boost/yap/user_macros.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <boost::yap::expr_kind Kind, typename Tuple> struct statement {
  static const boost::yap::expr_kind kind = Kind;

  Tuple elements;
};

BOOST_YAP_USER_BINARY_OPERATOR(comma, statement, statement);

} // namespace prtcl::expr

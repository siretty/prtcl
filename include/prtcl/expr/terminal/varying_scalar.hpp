#pragma once

#include "../prtcl_domain.hpp"

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct varying_scalar {
  Value value;

  friend std::ostream &operator<<(std::ostream &s,
                                  varying_scalar<Value> const &v) {
    s << "varying_scalar(";
    if constexpr (std::is_convertible<Value, std::string>::value)
      s << v.value;
    else
      s << "...";
    return s << ")";
  }
};

template <typename> struct is_varying_scalar : std::false_type {};

template <typename Value>
struct is_varying_scalar<varying_scalar<Value>> : std::true_type {};

template <typename Value>
using varying_scalar_term =
    prtcl_expr<typename boost::proto::terminal<varying_scalar<Value>>::type>;

struct VaryingScalarTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_varying_scalar<boost::proto::_value>()>> {};

} // namespace prtcl::expr

#pragma once

#include "../prtcl_domain.hpp"

#include <ostream>
#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct uniform_scalar {
  Value value;

  friend std::ostream &operator<<(std::ostream &s,
                                  uniform_scalar<Value> const &v) {
    s << "uniform_scalar(";
    if constexpr (std::is_convertible<Value, std::string>::value)
      s << v.value;
    else
      s << "...";
    return s << ")";
  }
};

template <typename> struct is_uniform_scalar : std::false_type {};

template <typename Value>
struct is_uniform_scalar<uniform_scalar<Value>> : std::true_type {};

template <typename Value>
using uniform_scalar_term =
    prtcl_expr<typename boost::proto::terminal<uniform_scalar<Value>>::type>;

struct UniformScalarTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_uniform_scalar<boost::proto::_value>()>> {};

} // namespace prtcl::expr

#pragma once

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct varying_scalar { Value value; };

template <typename> struct is_varying_scalar : std::false_type {};

template <typename Value>
struct is_varying_scalar<varying_scalar<Value>> : std::true_type {};

template <typename Value>
using varying_scalar_term =
    typename boost::proto::terminal<varying_scalar<Value>>::type;

struct VaryingScalarTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_varying_scalar<boost::proto::_value>()>> {};

} // namespace prtcl::expr

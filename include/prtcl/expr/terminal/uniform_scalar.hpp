#pragma once

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct uniform_scalar { Value value; };

template <typename> struct is_uniform_scalar : std::false_type {};

template <typename Value>
struct is_uniform_scalar<uniform_scalar<Value>> : std::true_type {};

template <typename Value>
using uniform_scalar_term =
    typename boost::proto::terminal<uniform_scalar<Value>>::type;

struct UniformScalarTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_uniform_scalar<boost::proto::_value>()>> {};

} // namespace prtcl::expr

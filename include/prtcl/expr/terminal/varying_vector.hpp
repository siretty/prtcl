#pragma once

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct varying_vector { Value value; };

template <typename> struct is_varying_vector : std::false_type {};

template <typename Value>
struct is_varying_vector<varying_vector<Value>> : std::true_type {};

template <typename Value>
using varying_vector_term =
    typename boost::proto::terminal<varying_vector<Value>>::type;

struct VaryingVectorTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_varying_vector<boost::proto::_value>()>> {};

} // namespace prtcl::expr

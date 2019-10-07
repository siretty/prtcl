#pragma once

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct uniform_vector { Value value; };

template <typename> struct is_uniform_vector : std::false_type {};

template <typename Value>
struct is_uniform_vector<uniform_vector<Value>> : std::true_type {};

template <typename Value>
using uniform_vector_term =
    typename boost::proto::terminal<uniform_vector<Value>>::type;

struct UniformVectorTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_uniform_vector<boost::proto::_value>()>> {};

} // namespace prtcl::expr

#pragma once

#include "../prtcl_domain.hpp"

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct uniform_vector { Value value; 

  friend std::ostream &operator<<(std::ostream &s,
                                  uniform_vector<Value> const &v) {
    s << "uniform_vector(";
    if constexpr (std::is_convertible<Value, std::string>::value)
      s << v.value;
    else
      s << "...";
    return s << ")";
  }
};

template <typename> struct is_uniform_vector : std::false_type {};

template <typename Value>
struct is_uniform_vector<uniform_vector<Value>> : std::true_type {};

template <typename Value>
using uniform_vector_term =
    prtcl_expr<typename boost::proto::terminal<uniform_vector<Value>>::type>;

struct UniformVectorTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_uniform_vector<boost::proto::_value>()>> {};

} // namespace prtcl::expr

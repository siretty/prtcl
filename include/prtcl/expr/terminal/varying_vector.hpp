#pragma once

#include "../prtcl_domain.hpp"

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename Value> struct varying_vector {
  Value value;

  friend std::ostream &operator<<(std::ostream &s,
                                  varying_vector<Value> const &v) {
    s << "varying_vector(";
    if constexpr (std::is_convertible<Value, std::string>::value)
      s << v.value;
    else
      s << "...";
    return s << ")";
  }
};

template <typename> struct is_varying_vector : std::false_type {};

template <typename Value>
struct is_varying_vector<varying_vector<Value>> : std::true_type {};

template <typename Value>
using varying_vector_term =
    prtcl_expr<typename boost::proto::terminal<varying_vector<Value>>::type>;

struct VaryingVectorTerm
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_varying_vector<boost::proto::_value>()>> {};

} // namespace prtcl::expr

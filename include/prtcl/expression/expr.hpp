#pragma once

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

template <typename> struct expr;

struct expr_generator : boost::proto::pod_generator<expr> {};

struct expr_domain : boost::proto::domain<expr_generator> {
  template <typename T> struct as_child : proto_base_domain::as_expr<T> {};
};

template <typename E> struct expr {
  BOOST_PROTO_EXTENDS(E, expr<E>, expr_domain);
};

} // namespace expression
} // namespace prtcl

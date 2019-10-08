#pragma once

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

template <typename E> struct prtcl_expr;

struct prtcl_generator : boost::proto::pod_generator<prtcl_expr> {};

struct prtcl_domain : boost::proto::domain<prtcl_generator> {
  template <typename T> struct as_child : proto_base_domain::as_expr<T> {};
};

template <typename E> struct prtcl_expr {
  BOOST_PROTO_EXTENDS(E, prtcl_expr<E>, prtcl_domain)
};

} // namespace prtcl::expr

#pragma once

#include "../terminal/active_index.hpp"
#include "../terminal/neighbour_index.hpp"
#include "../terminal/uniform_scalar.hpp"
#include "../terminal/varying_scalar.hpp"

#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct Scalar;

struct ScalarCases {
  // primary template matches nothing
  template <typename Tag> struct case_ : boost::proto::not_<boost::proto::_> {};
};

template <>
struct ScalarCases::case_<boost::proto::tag::terminal>
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<std::is_arithmetic<boost::proto::_value>()>> {};

template <>
struct ScalarCases::case_<boost::proto::tag::subscript>
    : boost::proto::subscript<
          boost::proto::or_<UniformScalarTerm, VaryingScalarTerm>,
          boost::proto::or_<active_index_term, neighbour_index_term>> {};

template <>
struct ScalarCases::case_<boost::proto::tag::unary_plus>
    : boost::proto::unary_plus<Scalar> {};

template <>
struct ScalarCases::case_<boost::proto::tag::negate>
    : boost::proto::negate<Scalar> {};

template <>
struct ScalarCases::case_<boost::proto::tag::plus>
    : boost::proto::plus<Scalar, Scalar> {};

template <>
struct ScalarCases::case_<boost::proto::tag::minus>
    : boost::proto::minus<Scalar, Scalar> {};

template <>
struct ScalarCases::case_<boost::proto::tag::multiplies>
    : boost::proto::multiplies<Scalar, Scalar> {};

template <>
struct ScalarCases::case_<boost::proto::tag::divides>
    : boost::proto::divides<Scalar, Scalar> {};

struct Scalar : boost::proto::switch_<ScalarCases> {};

struct ScalarAssignment
    : boost::proto::assign<
          boost::proto::subscript<
              boost::proto::or_<UniformScalarTerm, VaryingScalarTerm>,
              active_index_term>,
          Scalar> {};

struct ScalarAccumulation
    : boost::proto::plus_assign<
          boost::proto::subscript<
              boost::proto::or_<UniformScalarTerm, VaryingScalarTerm>,
              active_index_term>,
          Scalar> {};

} // namespace prtcl::expr

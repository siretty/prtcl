#pragma once

#include "../terminal/active_index.hpp"
#include "../terminal/neighbour_index.hpp"
#include "../terminal/uniform_vector.hpp"
#include "../terminal/varying_vector.hpp"

#include "scalar.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

struct Vector;

struct VectorCases {
  // primary template matches nothing
  template <typename Tag> struct case_ : boost::proto::not_<boost::proto::_> {};
};

template <>
struct VectorCases::case_<boost::proto::tag::subscript>
    : boost::proto::subscript<
          boost::proto::or_<UniformVectorTerm, VaryingVectorTerm>,
          boost::proto::or_<active_index_term, neighbour_index_term>> {};

template <>
struct VectorCases::case_<boost::proto::tag::unary_plus>
    : boost::proto::unary_plus<Vector> {};

template <>
struct VectorCases::case_<boost::proto::tag::negate>
    : boost::proto::negate<Vector> {};

template <>
struct VectorCases::case_<boost::proto::tag::plus>
    : boost::proto::or_<boost::proto::plus<Vector, Vector>,
                        boost::proto::plus<Vector, Scalar>,
                        boost::proto::plus<Scalar, Vector>> {};

template <>
struct VectorCases::case_<boost::proto::tag::minus>
    : boost::proto::or_<boost::proto::minus<Vector, Vector>,
                        boost::proto::minus<Vector, Scalar>,
                        boost::proto::minus<Scalar, Vector>> {};

template <>
struct VectorCases::case_<boost::proto::tag::multiplies>
    : boost::proto::or_<boost::proto::multiplies<Vector, Vector>,
                        boost::proto::multiplies<Vector, Scalar>,
                        boost::proto::multiplies<Scalar, Vector>> {};

template <>
struct VectorCases::case_<boost::proto::tag::divides>
    : boost::proto::or_<boost::proto::divides<Vector, Vector>,
                        boost::proto::divides<Vector, Scalar>,
                        boost::proto::divides<Scalar, Vector>> {};

struct Vector : boost::proto::switch_<VectorCases> {};

struct VectorAssignment
    : boost::proto::assign<
          boost::proto::subscript<
              boost::proto::or_<UniformVectorTerm, VaryingVectorTerm>,
              active_index_term>,
          Vector> {};

struct VectorAccumulation
    : boost::proto::plus_assign<
          boost::proto::subscript<
              boost::proto::or_<UniformVectorTerm, VaryingVectorTerm>,
              active_index_term>,
          Vector> {};

} // namespace prtcl::expr

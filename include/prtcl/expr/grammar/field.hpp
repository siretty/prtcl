#pragma once

#include "../terminal/uniform_scalar.hpp"
#include "../terminal/uniform_vector.hpp"
#include "../terminal/varying_scalar.hpp"
#include "../terminal/varying_vector.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

using AnyFieldTerm = boost::proto::or_<UniformScalarTerm, UniformVectorTerm,
                                       VaryingScalarTerm, VaryingVectorTerm>;

} // namespace prtcl::expr

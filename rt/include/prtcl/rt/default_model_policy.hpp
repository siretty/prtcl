#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/vector_data_policy.hpp>

namespace prtcl::rt {

template <size_t Dimensionality_>
using default_model_policy = basic_model_policy<
    fib_type_policy, eigen_math_policy, vector_data_policy, Dimensionality_>;

} // namespace prtcl::rt

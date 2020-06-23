#include "scheme_base.hpp"
#include "viscosity.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_viscosity() {
  (void)(viscosity<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(viscosity<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

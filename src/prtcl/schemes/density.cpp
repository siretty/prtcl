#include "scheme_base.hpp"
#include "density.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_density() {
  (void)(density<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(density<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

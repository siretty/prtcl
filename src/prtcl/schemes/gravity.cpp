#include "scheme_base.hpp"
#include "gravity.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_gravity() {
  (void)(gravity<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(gravity<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

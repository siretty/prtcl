#include "scheme_base.hpp"
#include "aat13.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_aat13() {
  (void)(aat13<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(aat13<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

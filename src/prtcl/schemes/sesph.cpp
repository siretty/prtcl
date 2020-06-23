#include "scheme_base.hpp"
#include "sesph.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_sesph() {
  (void)(sesph<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(sesph<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

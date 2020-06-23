#include "scheme_base.hpp"
#include "iisph.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_iisph() {
  (void)(iisph<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(iisph<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

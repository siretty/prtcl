#include "scheme_base.hpp"
#include "wkbb18.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_wkbb18() {
  (void)(wkbb18<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(wkbb18<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

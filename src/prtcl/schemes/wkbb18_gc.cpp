#include "scheme_base.hpp"
#include "wkbb18_gc.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_wkbb18_gc() {
  (void)(wkbb18_gc<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(wkbb18_gc<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

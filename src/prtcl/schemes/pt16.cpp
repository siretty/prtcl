#include "scheme_base.hpp"
#include "pt16.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_pt16() {
  (void)(pt16<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(pt16<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

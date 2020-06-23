#include "scheme_base.hpp"
#include "correction.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_correction() {
  (void)(correction<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(correction<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

#include "scheme_base.hpp"
#include "he14.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_he14() {
  (void)(he14<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(he14<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

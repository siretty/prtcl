#include "scheme_base.hpp"
#include "aiast12.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_aiast12() {
  (void)(aiast12<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(aiast12<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

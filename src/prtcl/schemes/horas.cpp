#include "scheme_base.hpp"
#include "horas.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_horas() {
  (void)(horas<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(horas<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

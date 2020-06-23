#include "scheme_base.hpp"
#include "symplectic_euler.hpp"

#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl { namespace schemes {

void Register_symplectic_euler() {
  (void)(symplectic_euler<float, 3, prtcl::math::cubic_spline_kernel>::registered_);
  (void)(symplectic_euler<double, 3, prtcl::math::cubic_spline_kernel>::registered_);
}

} }

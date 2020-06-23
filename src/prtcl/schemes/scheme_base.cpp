#include "scheme_base.hpp"

namespace prtcl {

SchemeRegistry &GetSchemeRegistry() {
  static auto *instance = new SchemeRegistry;
  return *instance;
}

} // namespace prtcl

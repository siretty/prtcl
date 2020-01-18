#include <prtcl/gt/schemes_registry.hpp>

namespace prtcl::gt {

schemes_registry &get_schemes_registry() {
  static auto *reg = new schemes_registry{};
  return *reg;
}

} // namespace prtcl::gt

#include "varying_manager.hpp"

namespace prtcl {

// CollectionOfMutableTensors const &
// VaryingManager::AddField(std::string_view name, TensorType type) {
//  PRTCL_DISPATCH_TENSOR_TYPE(type, return AddFieldImpl, name)
//}

void VaryingManager::AddField(std::string_view name, TensorType type) {
  log::Debug(
      "lib", "VaryingManager::AddField",
      "ttype.rank=", type.GetShape().GetRank(),
      " ctype=", type.GetComponentType().ToStringView());
  PRTCL_DISPATCH_TENSOR_TYPE(type, AddFieldImpl, name)
}

} // namespace prtcl

#include "varying_manager.hpp"

namespace prtcl {

VaryingField VaryingManager::AddField(std::string_view name, TensorType type) {
  log::Debug(
      "lib", "VaryingManager::AddField",
      "ttype.rank=", type.GetShape().GetRank(),
      " ctype=", type.GetComponentType().ToStringView());
  PRTCL_DISPATCH_TENSOR_TYPE(type, AddFieldImpl, name)
  return (fields_.find(name)->second);
}

VaryingField VaryingManager::GetField(std::string_view name) {
  if (auto it = fields_.find(name); it != fields_.end())
    return (it->second);
  else
    throw FieldDoesNotExist{};
}

} // namespace prtcl

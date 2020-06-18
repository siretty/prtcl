#include "uniform_manager.hpp"

namespace prtcl {

UniformField UniformManager::AddField(std::string_view name, TensorType type) {
  log::Debug(
      "lib", "UniformManager::AddField",
      "ttype.rank=", type.GetShape().GetRank(),
      " ctype=", type.GetComponentType().ToStringView());
  PRTCL_DISPATCH_TENSOR_TYPE(type, AddFieldImpl, name)
  return (fields_.find(name)->second);
}

UniformField UniformManager::GetField(std::string_view name) {
  if (auto it = fields_.find(name); it != fields_.end())
    return (it->second);
  else
    throw FieldDoesNotExist{};
}

} // namespace prtcl

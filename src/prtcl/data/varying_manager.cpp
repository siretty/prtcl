#include "varying_manager.hpp"

namespace prtcl {

CollectionOfMutableTensors const &
VaryingManager::AddField(std::string_view name, TensorType type) {
  PRTCL_DISPATCH_TENSOR_TYPE(type, return AddFieldImpl, name)
}

} // namespace prtcl

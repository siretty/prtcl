#include "tensor_type.hpp"

#include <ostream>

namespace prtcl {

void TensorType::Save(ArchiveWriter &archive) const {
  this->ctype_.Save(archive);
  this->shape_.Save(archive);
}

void TensorType::Load(ArchiveReader &archive) {
  this->ctype_.Load(archive);
  this->shape_.Load(archive);
}

std::ostream &operator<<(std::ostream &o, TensorType const &v) {
  return (o << v.GetComponentType() << v.GetShape());
}

} // namespace prtcl

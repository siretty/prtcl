#include "tensor_type.hpp"

#include <ostream>

namespace prtcl {

std::ostream &operator<<(std::ostream &o, TensorType const &v) {
  return (o << v.GetComponentType() << v.GetShape());
}

} // namespace prtcl

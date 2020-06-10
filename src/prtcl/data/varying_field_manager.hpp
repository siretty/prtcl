#ifndef PRTCL_DATA_VARYING_FIELD_MANAGER_HPP
#define PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

#include <prtcl/tensors.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace prtcl {

class VaryingFieldManager {
private:
  std::unordered_map<std::string, std::unique_ptr<Tensors>> varying_;
};

} // namespace prtcl

#endif // PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

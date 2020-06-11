#ifndef PRTCL_MODEL_HPP
#define PRTCL_MODEL_HPP

#include "collection_of_mutable_tensors.hpp"
#include "group.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace prtcl {

class Model {
public:
  auto &AddGroup(std::string name, std::string type) {
    // TODO auto &AddGroup(string, string)
    return *this;
  }

  auto &GetGroup(size_t index) {
    // TODO auto &GetGroup(size_t)
    return *this;
  }

  auto Groups() {
    // TODO auto Groups()
  }

public:
  template <typename T, size_t... N>
  auto AddGlobal(std::string name) {
    // TODO auto AddGlobal(string)
  }

  auto Globals() {
    // TODO auto Globals()
  }

private:
  std::unordered_map<std::string, std::unique_ptr<CollectionOfMutableTensors>> global_;

  std::vector<std::unique_ptr<Group>> groups_;
  std::unordered_map<std::string, size_t> group_name_to_index_;
};

} // namespace prtcl

#endif // PRTCL_MODEL_HPP

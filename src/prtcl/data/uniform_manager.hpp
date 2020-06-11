#ifndef PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP
#define PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

#include "vector_of_tensors.hpp"

#include <memory>
#include <unordered_map>

#include <boost/range/adaptor/map.hpp>

namespace prtcl {

class UniformManager {
public:
  template <typename T, size_t... N>
  using ColT = VectorOfTensors<T, N...>;

  template <typename T, size_t... N>
  using SeqT = AccessToVectorOfTensors<T, N...>;

public:
  template <typename T, size_t... N>
  ColT<T, N...> const &AddField(std::string name) {
    auto [it, inserted] = fields_.emplace(name, nullptr);
    if (inserted)
      it->second.reset(new ColT<T, N...>);
    auto *col = static_cast<ColT<T, N...> *>(it->second.get());
    col->Resize(1);
    return *col;
  }

  template <typename T, size_t... N>
  ColT<T, N...> const *GetField(std::string name) const {
    if (auto it = fields_.find(name); it != fields_.end())
      return static_cast<ColT<T, N...> *>(it->second.get());
    else
      return nullptr;
  }

  void RemoveField(std::string name) { fields_.erase(name); }

public:
  size_t GetFieldCount() const { return fields_.size(); }

  auto GetNames() const {
    return boost::make_iterator_range(fields_) | boost::adaptors::map_keys;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<CollectionOfMutableTensors>>
      fields_;
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

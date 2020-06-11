#ifndef PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP
#define PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

#include "../errors/field_does_not_exist.hpp"
#include "../errors/field_of_different_type_already_exists_error.hpp"
#include "../errors/invalid_identifier_error.hpp"
#include "../is_valid_identifier.hpp"
#include "vector_of_tensors.hpp"

#include <memory>
#include <unordered_map>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace prtcl {

class UniformManager {
public:
  template <typename T, size_t... N>
  using ColT = VectorOfTensors<T, N...>;

  template <typename T, size_t... N>
  using SeqT = AccessToVectorOfTensors<T, N...>;

public:
  template <typename T, size_t... N>
  ColT<T, N...> const &AddField(std::string_view name) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto [it, inserted] = fields_.emplace(std::string{name}, nullptr);
    if (inserted)
      it->second.reset(new ColT<T, N...>);
    else if (it->second.get()->GetType() != GetTensorTypeCRef<T, N...>())
      throw FieldOfDifferentTypeAlreadyExistsError{};

    auto *col = static_cast<ColT<T, N...> *>(it->second.get());
    col->Resize(1);
    return *col;
  }

  template <typename T, size_t... N>
  ColT<T, N...> const *TryGetField(std::string name) const {
    if (auto it = fields_.find(name); it != fields_.end()) {
      if (auto *field = it->second.get();
          field->GetType() == GetTensorTypeCRef<T, N...>())
        return static_cast<ColT<T, N...> *>(it->second.get());
      else
        throw FieldOfDifferentTypeAlreadyExistsError{};
    } else
      return nullptr;
  }

  void RemoveField(std::string_view name) {
    if (auto it = fields_.find(name); it != fields_.end())
      fields_.erase(it);
  }

  bool HasField(std::string_view name) {
    return fields_.find(name) != fields_.end();
  }

public:
  size_t GetFieldCount() const { return fields_.size(); }

public:
  auto GetNamedFields() const {
    return boost::make_iterator_range(fields_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<
                 std::string const &, CollectionOfMutableTensors const &>{
                 entry.first, *entry.second.get()};
           });
  }

  auto GetNames() const { return GetNamedFields() | boost::adaptors::map_keys; }

  auto GetFields() const {
    return GetNamedFields() | boost::adaptors::map_values;
  }

private:
  boost::container::flat_map<
      std::string, std::unique_ptr<CollectionOfMutableTensors>, std::less<>>
      fields_ = {};
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

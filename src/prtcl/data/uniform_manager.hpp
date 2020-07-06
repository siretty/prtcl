#ifndef PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP
#define PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

#include "../cxx/map.hpp"
#include "../errors/field_does_not_exist.hpp"
#include "../errors/field_of_different_type_already_exists_error.hpp"
#include "../errors/invalid_identifier_error.hpp"
#include "../errors/not_implemented_error.hpp"
#include "prtcl/util/is_valid_identifier.hpp"
#include "uniform_field.hpp"

#include <memory>
#include <unordered_map>

#include <cstddef>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace prtcl {

class UniformManager {
public:
  template <typename T, size_t... N>
  UniformFieldSpan<T, N...> AddFieldImpl(std::string_view name) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto [it, inserted] =
        fields_.emplace(std::string{name}, MakeUniformField<T, N...>());
    if (not inserted and it->second.GetType() != GetTensorTypeCRef<T, N...>())
      throw FieldOfDifferentTypeAlreadyExistsError{};

    return it->second.template Span<T, N...>();
  }

  UniformField AddField(std::string_view name, TensorType type);

  UniformField GetField(std::string_view name);

  template <typename T, size_t... N>
  UniformFieldSpan<T, N...> FieldSpan(std::string_view name) const {
    if (auto it = fields_.find(name); it != fields_.end()) {
      return it->second.Span<T, N...>();
    } else
      throw FieldDoesNotExist{};
  }

  template <typename U, size_t... N>
  UniformFieldWrap<U, N...> FieldWrap(std::string_view name) const {
    if (auto it = fields_.find(name); it != fields_.end()) {
      return it->second.Wrap<U, N...>();
    } else
      throw FieldDoesNotExist{};
  }

  void RemoveField(std::string_view name) {
    if (auto it = fields_.find(name); it != fields_.end())
      fields_.erase(it);
  }

  bool HasField(std::string_view name) const {
    return fields_.find(name) != fields_.end();
  }

public:
  size_t GetFieldCount() const { return fields_.size(); }

public:
  auto GetNamedFields() const {
    return boost::make_iterator_range(fields_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<std::string const &, UniformField const &>{
                 entry.first, entry.second};
           });
  }

  auto GetFieldNames() const {
    return GetNamedFields() | boost::adaptors::map_keys;
  }

  auto GetFields() const {
    return GetNamedFields() | boost::adaptors::map_values;
  }

public:
  void Save(ArchiveWriter &archive) const;

  void Load(ArchiveReader &archive);

private:
  cxx::het_flat_map<std::string, UniformField> fields_ = {};
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_UNIFORM_MANAGER_HPP

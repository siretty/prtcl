#ifndef PRTCL_DATA_VARYING_FIELD_MANAGER_HPP
#define PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

#include "../cxx/map.hpp"
#include "../errors/field_does_not_exist.hpp"
#include "../errors/field_of_different_type_already_exists_error.hpp"
#include "../errors/invalid_identifier_error.hpp"
#include "collection_of_mutable_tensors.hpp"
#include "prtcl/util/is_valid_identifier.hpp"
#include "varying_field.hpp"
#include "vector_of_tensors.hpp"

#include "../log.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy_backward.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl {

class VaryingManager {
public:
  template <typename T, size_t... N>
  VaryingFieldSpan<T, N...> AddFieldImpl(std::string_view name) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto [it, inserted] =
        fields_.emplace(std::string{name}, MakeVaryingField<T, N...>());
    if (not inserted and it->second.GetType() != GetTensorTypeCRef<T, N...>())
      throw FieldOfDifferentTypeAlreadyExistsError{};

    it->second.Resize(GetItemCount());
    return it->second.template Span<T, N...>();
  }

  VaryingField AddField(std::string_view name, TensorType type);

  VaryingField GetField(std::string_view name);

  template <typename T, size_t... N>
  VaryingFieldSpan<T, N...> FieldSpan(std::string_view name) const {
    if (auto it = fields_.find(name); it != fields_.end()) {
      return it->second.Span<T, N...>();
    } else
      throw FieldDoesNotExist{};
  }

  template <typename U, size_t... N>
  VaryingFieldWrap<U, N...> FieldWrap(std::string_view name) const {
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
  void ResizeItems(size_t new_size) {
    for (auto &[name, field] : fields_)
      field.Resize(new_size);

    size_ = new_size;

    SetDirty(true);
  }

  void PermuteItems(cxx::span<size_t const> input_perm) {
    thread_local std::vector<size_t> work_perm;
    work_perm.reserve(GetItemCount());
#pragma omp for schedule(dynamic)
    for (auto &[name, field] : fields_) {
      assert(GetItemCount() == field.GetSize());
      work_perm.clear();
      boost::copy(input_perm, std::back_inserter(work_perm));
      field.ConsumePermutation(work_perm);
    }

    // set the dirty flag if particles were reordered
    SetDirty(true);
  }

public:
  boost::integer_range<size_t> CreateItems(size_t count) {
    size_t const old_size = GetItemCount();
    size_t const new_size = old_size + count;
    ResizeItems(new_size);
    return boost::irange(old_size, new_size);
  }

  void DestroyItems(cxx::span<size_t const> indices) {
    if (indices.size() == 0)
      return;

    // ensure the permutation buffer is big enough
    destroy_items_perm_.resize(GetItemCount());

    // copy and sort the indices that are to be destroyed
    auto first = boost::copy_backward(indices, destroy_items_perm_.end());
    std::sort(first, destroy_items_perm_.end());

    // generate the remaining indices of the permutation
    boost::range::set_difference(
        boost::irange<size_t>(0, GetItemCount()),
        boost::make_iterator_range(first, destroy_items_perm_.end()),
        destroy_items_perm_.begin());

    // permute this groups fields such that all particles that are to be
    // destroyed are in the end
    PermuteItems(destroy_items_perm_);

    // remove the particles by resizing this group
    ResizeItems(GetItemCount() - indices.size());
  }

private:
  std::vector<size_t> destroy_items_perm_;

public:
  bool IsDirty() const { return dirty_; }

  void SetDirty(bool value) { dirty_ = value; }

public:
  size_t GetFieldCount() const { return fields_.size(); }

  size_t GetItemCount() const { return size_; }

public:
  auto GetNamedFields() const {
    return boost::make_iterator_range(fields_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<
                 // std::string const &, CollectionOfMutableTensors const &>{
                 std::string const &, VaryingField const &>{
                 entry.first, entry.second};
           });
  }

  auto GetFieldNames() const {
    return GetNamedFields() | boost::adaptors::map_keys;
  }

  auto GetFields() const {
    return GetNamedFields() | boost::adaptors::map_values;
  }

private:
  size_t size_ = 0;
  bool dirty_ = false;

  cxx::het_flat_map<std::string, VaryingField> fields_;
};

} // namespace prtcl

#endif // PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

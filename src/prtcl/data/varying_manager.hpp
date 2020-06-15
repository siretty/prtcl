#ifndef PRTCL_DATA_VARYING_FIELD_MANAGER_HPP
#define PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

#include "../errors/field_does_not_exist.hpp"
#include "../errors/field_of_different_type_already_exists_error.hpp"
#include "../errors/invalid_identifier_error.hpp"
#include "collection_of_mutable_tensors.hpp"
#include "prtcl/util/is_valid_identifier.hpp"
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
  using ColT = VectorOfTensors<T, N...>;

  template <typename T, size_t... N>
  using SeqT = AccessToVectorOfTensors<T, N...>;

public:
  template <typename T, size_t... N>
  ColT<T, N...> const &AddFieldImpl(std::string_view name) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto [it, inserted] = fields_.emplace(std::string{name}, nullptr);
    if (inserted)
      it->second.reset(new ColT<T, N...>);
    else if (it->second.get()->GetType() != GetTensorTypeCRef<T, N...>())
      throw FieldOfDifferentTypeAlreadyExistsError{};

    auto *col = static_cast<ColT<T, N...> *>(it->second.get());
    col->Resize(GetItemCount());
    return *col;
  }

  CollectionOfMutableTensors const &
  AddField(std::string_view name, TensorType type);

  template <typename T, size_t... N>
  ColT<T, N...> const *TryGetFieldImpl(std::string_view name) const {
    if (auto const *field = TryGetField(name))
      if (field->GetType() == GetTensorTypeCRef<T, N...>())
        return static_cast<ColT<T, N...> const *>(field);
      else
        return nullptr;
    else
      return nullptr;
  }

  CollectionOfMutableTensors const *TryGetField(std::string_view name) const {
    if (auto it = fields_.find(name); it != fields_.end())
      return it->second.get();
    else
      return nullptr;
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
      field->Resize(new_size);

    size_ = new_size;

    SetDirty(true);
  }

  void PermuteItems(cxx::span<size_t const> input_perm) {
    thread_local std::vector<size_t> work_perm;
    work_perm.reserve(GetItemCount());
#pragma omp for schedule(dynamic)
    for (auto &[name, field] : fields_) {
      assert(GetItemCount() == field->GetSize());
      work_perm.clear();
      boost::copy(input_perm, std::back_inserter(work_perm));
      field->Permute(work_perm);
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
                 std::string const &, CollectionOfMutableTensors const &>{
                 entry.first, *entry.second.get()};
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

  boost::container::flat_map<
      std::string, std::unique_ptr<CollectionOfMutableTensors>, std::less<>>
      fields_ = {};
};

} // namespace prtcl

#endif // PRTCL_DATA_VARYING_FIELD_MANAGER_HPP

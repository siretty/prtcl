#ifndef PRTCL_GROUP_HPP
#define PRTCL_GROUP_HPP

#include "../cxx/set.hpp"
#include "../errors/field_of_different_kind_already_exists_error.hpp"
#include "../log.hpp"
#include "uniform_manager.hpp"
#include "varying_manager.hpp"

#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/container/flat_set.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl {

class Model;

enum class GroupIndex : size_t {
  kNoIndex = std::numeric_limits<size_t>::max()
};

std::ostream &operator<<(std::ostream &, GroupIndex);

class Group {
public:
  Group() = delete;

  Group(Group &&) = default;
  Group &operator=(Group &&) = default;

  Group(
      Model &model, std::string_view name, std::string_view type,
      GroupIndex index = GroupIndex::kNoIndex);

public:
  Model const &GetModel() const { return *model_; }

public:
  std::string_view GetGroupName() const { return name_; }

  std::string_view GetGroupType() const { return type_; }

  GroupIndex GetGroupIndex() const { return index_; }

  void SetGroupIndex(GroupIndex value) { index_ = value; }

public:
  size_t GetItemCount() const { return varying_.GetItemCount(); }

public:
  auto GetTags() const { return boost::make_iterator_range(tags_); }

  bool HasTag(std::string const &tag) const {
    return tags_.find(tag) != tags_.end();
  }

  void AddTag(std::string const &tag) { tags_.insert(tag); }

  void RemoveTag(std::string const &tag) { tags_.erase(tag); }

public:
  UniformManager const &GetUniform() const { return uniform_; }

  template <typename T, size_t... N>
  UniformFieldSpan<T, N...> AddUniformFieldImpl(std::string_view name) {
    if (not varying_.HasField(std::string{name}))
      return uniform_.AddFieldImpl<T, N...>(name);
    else
      throw FieldOfDifferentKindAlreadyExistsError{};
  }

  UniformField AddUniformField(std::string_view name, TensorType type) {
    if (not varying_.HasField(std::string{name}))
      return uniform_.AddField(name, type);
    else
      throw FieldOfDifferentKindAlreadyExistsError{};
  }

public:
  VaryingManager const &GetVarying() const { return varying_; }

  template <typename T, size_t... N>
  VaryingFieldSpan<T, N...> AddVaryingFieldImpl(std::string_view name) {
    if (not uniform_.HasField(name))
      return varying_.AddFieldImpl<T, N...>(std::string{name});
    else
      throw FieldOfDifferentKindAlreadyExistsError{};
  }

  VaryingField AddVaryingField(std::string_view name, TensorType type) {
    log::Debug(
        "lib", "Group::AddVaryingField",
        "ttype.rank=", type.GetShape().GetRank(),
        " ctype=", type.GetComponentType().ToStringView());
    if (not uniform_.HasField(name)) {
      auto field = varying_.AddField(name, type);
      return field;
    } else
      throw FieldOfDifferentKindAlreadyExistsError{};
  }

public:
  void RemoveField(std::string_view name) {
    uniform_.RemoveField(name);
    varying_.RemoveField(name);
  }

public:
  auto CreateItems(size_t count) { return varying_.CreateItems(count); }

  void DestroyItems(cxx::span<size_t const> indices) {
    varying_.DestroyItems(indices);
  }

  bool IsDirty() const { return varying_.IsDirty(); }

  void SetDirty(bool value) { varying_.SetDirty(value); }

public:
  void Resize(size_t new_size) { varying_.ResizeItems(new_size); }

  void Permute(cxx::span<size_t const> input_perm) {
    varying_.PermuteItems(input_perm);
  }

public:
  void Save(ArchiveWriter &archive) const;

  void Load(ArchiveReader &archive);

private:
  Model *model_;

  std::string name_;
  std::string type_;
  GroupIndex index_ = GroupIndex::kNoIndex;

  boost::container::flat_set<std::string> tags_ = {};

  VaryingManager varying_ = {};
  UniformManager uniform_ = {};
};

} // namespace prtcl

#endif // PRTCL_GROUP_HPP

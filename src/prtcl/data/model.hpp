#ifndef PRTCL_MODEL_HPP
#define PRTCL_MODEL_HPP

#include "../is_valid_identifier.hpp"
#include "group.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace prtcl {

class Model {
public:
  Group &AddGroup(std::string_view name, std::string_view type) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto [it, inserted] = groups_.emplace(std::string{name}, nullptr);
    if (inserted) {
      auto group_index = static_cast<GroupIndex>(groups_by_index_.size());
      it->second.reset(new Group{name, type, group_index});
      groups_by_index_.emplace_back(it->second.get());
    }

    return *it->second.get();
  }

  Group *TryGetGroup(std::string_view name) {
    if (auto it = groups_.find(name); it != groups_.end())
      return it->second.get();
    else
      return nullptr;
  }

  void RemoveGroup(std::string_view name) {
    if (auto it = groups_.find(name); it != groups_.end()) {
      auto &group_ptr = it->second;
      auto const group_index = static_cast<size_t>(group_ptr->GetGroupIndex());
      groups_by_index_[group_index] = nullptr;
      groups_.erase(it);
    }
  }

public:
  auto GetNamedGroups() {
    return boost::make_iterator_range(groups_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<std::string const &, Group &>{
                 entry.first, *entry.second.get()};
           });
  }

  auto GetGroupNames() { return GetNamedGroups() | boost::adaptors::map_keys; }

  auto GetGroups() { return GetNamedGroups() | boost::adaptors::map_values; }

public:
  auto GetNamedGroups() const {
    return boost::make_iterator_range(groups_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<std::string const &, Group const &>{
                 entry.first, *entry.second.get()};
           });
  }

  auto GetGroupNames() const {
    return GetNamedGroups() | boost::adaptors::map_keys;
  }

  auto GetGroups() const {
    return GetNamedGroups() | boost::adaptors::map_values;
  }

public:
  bool IsDirty() const {
    auto groups = GetGroups();
    return std::any_of(groups.begin(), groups.end(), [](auto &group) {
      return group.IsDirty();
    });
  }

  void SetDirty(bool value) {
    for (auto &group : GetGroups())
      group.SetDirty(value);
  }

public:
  size_t GetGroupCount() const { return groups_.size(); }

public:
  UniformManager const &GetGlobal() const { return global_; }

  template <typename T, size_t... N>
  auto const &AddGlobalField(std::string_view name) {
    return global_.AddField<T, N...>(name);
  }

  void RemoveGlobalField(std::string_view name) { global_.RemoveField(name); }

private:
  UniformManager global_;

  boost::container::flat_map<std::string, std::unique_ptr<Group>, std::less<>>
      groups_ = {};
  std::vector<Group *> groups_by_index_;
};

} // namespace prtcl

#endif // PRTCL_MODEL_HPP

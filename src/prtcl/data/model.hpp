#ifndef PRTCL_MODEL_HPP
#define PRTCL_MODEL_HPP

#include "../cxx/map.hpp"
#include "../cxx/set.hpp"
#include "group.hpp"
#include "prtcl/util/is_valid_identifier.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace prtcl {

class Model {
public:
  Model();

  Model(Model const &) = delete;
  Model &operator=(Model const &) = delete;

  Model(Model &&) = default;
  Model &operator=(Model &&) = default;

public:
  Group &AddGroup(std::string_view name, std::string_view type);

  Group *TryGetGroup(std::string_view name);

  void RemoveGroup(std::string_view name);

public:
  size_t GetGroupCount() const { return groups_.size(); }

public:
  auto GetNamedGroups() {
    return boost::make_iterator_range(groups_) |
           boost::adaptors::transformed([](auto const &entry) {
             return std::pair<std::string const &, Group &>{
                 entry.first, *entry.second.get()};
           });
  }

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
  UniformManager const &GetGlobal() const { return global_; }

  template <typename T, size_t... N>
  UniformFieldSpan<T, N...> AddGlobalFieldImpl(std::string_view name) {
    return global_.AddFieldImpl<T, N...>(name);
  }

  UniformField AddGlobalField(std::string_view name, TensorType type) {
    return global_.AddField(name, type);
  }

  void RemoveGlobalField(std::string_view name) { global_.RemoveField(name); }

public:
  void Save(ArchiveWriter &archive) const;

  void Load(ArchiveReader &archive);

private:
  UniformManager global_;

  cxx::het_flat_map<std::string, std::unique_ptr<Group>> groups_;
  std::vector<Group *> groups_by_index_;
};

} // namespace prtcl

#endif // PRTCL_MODEL_HPP

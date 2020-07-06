#include "model.hpp"

#include "../errors/group_of_different_type_already_exists.hpp"
#include "../log.hpp"

namespace prtcl {

PRTCL_DEFINE_LOG_FOR_INSTANCE(Debug, prtcl::data, Model)
PRTCL_DEFINE_LOG_FOR_INSTANCE(Warning, prtcl::data, Model)

Model::Model() { LogDebug(this, "Model()"); }

Group &Model::AddGroup(std::string_view name, std::string_view type) {
  if (not IsValidIdentifier(name))
    throw InvalidIdentifierError{};

  auto [it, inserted] = groups_.emplace(std::string{name}, nullptr);
  if (inserted) {
    auto group_index = static_cast<GroupIndex>(groups_by_index_.size());
    it->second.reset(new Group{*this, name, type});
    groups_by_index_.emplace_back(it->second.get());
  } else if (it->second->GetGroupType() != type) {
    throw GroupOfDifferentTypeAlreadyExists{};
  }

  // HACK: fix the GroupIndex field of every group
  for (size_t gi = 0; gi < groups_.size(); ++gi) {
    auto *group = groups_.begin()[gi].second.get();
    group->SetGroupIndex(static_cast<GroupIndex>(gi));
    groups_by_index_[gi] = group;
  }

  return *it->second.get();
}

Group *Model::TryGetGroup(std::string_view name) {
  if (auto it = groups_.find(name); it != groups_.end())
    return it->second.get();
  else
    return nullptr;
}

void Model::RemoveGroup(std::string_view name) {
  if (auto it = groups_.find(name); it != groups_.end()) {
    auto &group_ptr = it->second;
    auto const group_index = static_cast<size_t>(group_ptr->GetGroupIndex());
    groups_by_index_[group_index] = nullptr;
    groups_.erase(it);
  } else {
    LogWarning(this, "RemoveGroup(\"", name, "\"): group not found");
  }
}

void Model::Save(ArchiveWriter &archive) const {
  global_.Save(archive);

  archive.SaveSize(groups_.size());
  for (auto &[group_name, group_ptr] : groups_) {
    archive.SaveString(group_name);
    archive.SaveString(group_ptr->GetGroupType());
    group_ptr->Save(archive);
  }
}

void Model::Load(ArchiveReader &archive) {
  global_.Load(archive);

  size_t group_count = archive.LoadSize();
  for (size_t group_index = 0; group_index < group_count; ++group_index) {
    auto group_name = archive.LoadString();
    auto group_type = archive.LoadString();
    auto &group = this->AddGroup(group_name, group_type);
    group.Load(archive);
  }
}

} // namespace prtcl

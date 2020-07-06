#include "group.hpp"

#include "../errors/invalid_identifier_error.hpp"
#include "../log.hpp"
#include "prtcl/util/is_valid_identifier.hpp"

namespace prtcl {

PRTCL_DEFINE_LOG_FOR_INSTANCE(Debug, prtcl::data, Group)

std::ostream &operator<<(std::ostream &o, GroupIndex group_index) {
  switch (group_index) {
  case GroupIndex::kNoIndex:
    return (o << "GroupIndex::kNoIndex");
  default:
    return (o << "GroupIndex(" << static_cast<size_t>(group_index) << ")");
  }
}

Group::Group(
    Model &model, std::string_view name, std::string_view type,
    GroupIndex index)
    : model_{&model}, name_{name}, type_{type}, index_{index} {
  LogDebug(this, "Group(\"", name, "\", \"", type, "\", ", index, ")");
  if (not IsValidIdentifier(name_))
    throw InvalidIdentifierError{};
  if (not IsValidIdentifier(type_))
    throw InvalidIdentifierError{};
}

void Group::Save(ArchiveWriter &archive) const {
  // model_ and index_ are set in the constructor (on loading from a model)
  // name_ and type_ are stored by the model

  archive.SaveSize(this->tags_.size());
  for (auto const &tag : this->tags_) {
    archive.SaveString(tag);
  }

  this->varying_.Save(archive);
  this->uniform_.Save(archive);
}

void Group::Load(ArchiveReader &archive) {
  // model_ and index_ are set in the constructor (on loading from a model)
  // name_ and type_ are stored by the model

  LogDebug(this, "Load(", &archive, ") name_=", name_, " type_=", type_);

  size_t const tag_count = archive.LoadSize();
  for (size_t tag_index = 0; tag_index < tag_count; ++tag_index) {
    std::string tag = archive.LoadString();
    LogDebug(this, "Load(", &archive, ") tag=", tag);
    this->AddTag(tag);
  }

  this->varying_.Load(archive);
  this->uniform_.Load(archive);
}

} // namespace prtcl

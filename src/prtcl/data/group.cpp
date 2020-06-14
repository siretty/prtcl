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

Group::Group(std::string_view name, std::string_view type, GroupIndex index)
    : name_{name}, type_{type}, index_{index} {
  LogDebug(this, "Group(\"", name, "\", \"", type, "\", ", index, ")");
  if (not IsValidIdentifier(name_))
    throw InvalidIdentifierError{};
  if (not IsValidIdentifier(type_))
    throw InvalidIdentifierError{};
}

} // namespace prtcl

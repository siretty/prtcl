#include "uniform_manager.hpp"

namespace prtcl {

UniformField UniformManager::AddField(std::string_view name, TensorType type) {
  log::Debug(
      "lib", "UniformManager::AddField",
      "ttype.rank=", type.GetShape().GetRank(),
      " ctype=", type.GetComponentType().ToStringView());
  PRTCL_DISPATCH_TENSOR_TYPE(type, AddFieldImpl, name)
  return (fields_.find(name)->second);
}

UniformField UniformManager::GetField(std::string_view name) {
  if (auto it = fields_.find(name); it != fields_.end())
    return (it->second);
  else
    throw FieldDoesNotExist{};
}


void UniformManager::Save(ArchiveWriter &archive) const {
  archive.SaveSize(this->fields_.size());
  for (auto const &[name, field] : this->fields_) {
    archive.SaveString(name);
    field.GetType().Save(archive);
    field.Save(archive);
  }
}

void UniformManager::Load(ArchiveReader &archive) {
  size_t const field_count = archive.LoadSize();
  for (size_t field_index = 0; field_index < field_count; ++field_index) {
    auto const name = archive.LoadString();

    TensorType type;
    type.Load(archive);

    this->AddField(name, type).Load(archive);
  }
}

} // namespace prtcl

#include "archive.hpp"

namespace prtcl {

void NativeBinaryArchiveWriter::SaveSize(size_t size) {
  ostream_->write(reinterpret_cast<char const *>(&size), sizeof(size));
}

size_t NativeBinaryArchiveReader::LoadSize() {
  size_t result;
  istream_->read(reinterpret_cast<char *>(&result), sizeof(result));
  return result;
}

void NativeBinaryArchiveWriter::SaveString(std::string_view value) {
  this->SaveSize(value.size());
  ostream_->write(value.data(), value.size());
}

std::string NativeBinaryArchiveReader::LoadString() {
  auto const size = this->LoadSize();
  auto result = std::string(size, '@');
  istream_->read(result.data(), size);
  return result;
}

template <typename T>
void SaveValuesImpl(std::ostream &ostream, size_t count, T const *values) {
  for (size_t index = 0; index < count; ++index) {
    ostream.write(reinterpret_cast<char const *>(values + index), sizeof(T));
  }
}

template <typename T>
void LoadValuesImpl(std::istream &istream, size_t count, T *values) {
  for (size_t index = 0; index < count; ++index) {
    istream.read(reinterpret_cast<char *>(values + index), sizeof(T));
  }
}

void NativeBinaryArchiveWriter::SaveValues(size_t count, bool const *values) {
  SaveValuesImpl(*ostream_, count, values);
}

void NativeBinaryArchiveReader::LoadValues(size_t count, bool *values) {
  LoadValuesImpl(*istream_, count, values);
}

void NativeBinaryArchiveWriter::SaveValues(
    size_t count, int32_t const *values) {
  SaveValuesImpl(*ostream_, count, values);
}

void NativeBinaryArchiveReader::LoadValues(size_t count, int32_t *values) {
  LoadValuesImpl(*istream_, count, values);
}

void NativeBinaryArchiveWriter::SaveValues(
    size_t count, int64_t const *values) {
  SaveValuesImpl(*ostream_, count, values);
}

void NativeBinaryArchiveReader::LoadValues(size_t count, int64_t *values) {
  LoadValuesImpl(*istream_, count, values);
}

void NativeBinaryArchiveWriter::SaveValues(size_t count, float const *values) {
  SaveValuesImpl(*ostream_, count, values);
}

void NativeBinaryArchiveReader::LoadValues(size_t count, float *values) {
  LoadValuesImpl(*istream_, count, values);
}

void NativeBinaryArchiveWriter::SaveValues(size_t count, double const *values) {
  SaveValuesImpl(*ostream_, count, values);
}

void NativeBinaryArchiveReader::LoadValues(size_t count, double *values) {
  LoadValuesImpl(*istream_, count, values);
}

} // namespace prtcl

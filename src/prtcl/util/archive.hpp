#ifndef PRTCL_ARCHIVE_HPP_D87210E849204F6B926926BC738A7DEF
#define PRTCL_ARCHIVE_HPP_D87210E849204F6B926926BC738A7DEF

#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <cstdint>

namespace prtcl {

class ArchiveWriter {
public:
  virtual ~ArchiveWriter() = default;

public:
  virtual void SaveString(std::string_view value) = 0;
  virtual void SaveSize(size_t value) = 0;
  virtual void SaveValues(size_t count, bool const *values) = 0;
  virtual void SaveValues(size_t count, int32_t const *values) = 0;
  virtual void SaveValues(size_t count, int64_t const *values) = 0;
  virtual void SaveValues(size_t count, float const *values) = 0;
  virtual void SaveValues(size_t count, double const *values) = 0;
};

class ArchiveReader {
public:
  virtual ~ArchiveReader() = default;

public:
  virtual std::string LoadString() = 0;
  virtual size_t LoadSize() = 0;
  virtual void LoadValues(size_t count, bool *values) = 0;
  virtual void LoadValues(size_t count, int32_t *values) = 0;
  virtual void LoadValues(size_t count, int64_t *values) = 0;
  virtual void LoadValues(size_t count, float *values) = 0;
  virtual void LoadValues(size_t count, double *values) = 0;
};

class NativeBinaryArchiveWriter final : public ArchiveWriter {
public:
  void SaveSize(size_t value);
  void SaveString(std::string_view value);
  void SaveValues(size_t count, bool const *values);
  void SaveValues(size_t count, int32_t const *values);
  void SaveValues(size_t count, int64_t const *values);
  void SaveValues(size_t count, float const *values);
  void SaveValues(size_t count, double const *values);

public:
  NativeBinaryArchiveWriter(std::ostream &ostream) : ostream_{&ostream} {}

private:
  std::ostream *ostream_;
};

class NativeBinaryArchiveReader final : public ArchiveReader {
public:
  size_t LoadSize();
  std::string LoadString();
  void LoadValues(size_t count, bool *values);
  void LoadValues(size_t count, int32_t *values);
  void LoadValues(size_t count, int64_t *values);
  void LoadValues(size_t count, float *values);
  void LoadValues(size_t count, double *values);

public:
  NativeBinaryArchiveReader(std::istream &istream) : istream_{&istream} {}

private:
  std::istream *istream_;
};

} // namespace prtcl

#endif // PRTCL_ARCHIVE_HPP_D87210E849204F6B926926BC738A7DEF

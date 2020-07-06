#ifndef PRTCL_SRC_PRTCL_DATA_SHAPE_HPP
#define PRTCL_SRC_PRTCL_DATA_SHAPE_HPP

#include "../util/archive.hpp"
#include <prtcl/cxx.hpp>

#include <algorithm>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include <cassert>

#include <boost/operators.hpp>

namespace prtcl {

class Shape : public boost::equality_comparable<Shape> {
public:
  Shape() = default;

  Shape(cxx::span<size_t const> extents)
      : extents_{extents.begin(), extents.end()} {}

  Shape(std::initializer_list<size_t> extents) : extents_{extents} {}

public:
  size_t GetRank() const { return extents_.size(); }

  cxx::span<size_t const> GetExtents() const {
    return {extents_.data(), extents_.size()};
  }

  size_t operator[](size_t dim) const {
    assert(dim < GetRank());
    return extents_[dim];
  }

public:
  bool IsEmpty() const {
    return extents_.empty() or
           std::find(extents_.begin(), extents_.end(), 0) != extents_.end();
  }

public:
  void Save(ArchiveWriter &archive) const;

  void Load(ArchiveReader &archive);

public:
  friend bool operator==(Shape const &lhs, Shape const &rhs) {
    return lhs.extents_ == rhs.extents_;
  }

  friend std::ostream &operator<<(std::ostream &, Shape const &);

public:
  std::string ToString() const;

  static Shape FromString(std::string_view input);

private:
  std::vector<size_t> extents_ = {};
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_SHAPE_HPP

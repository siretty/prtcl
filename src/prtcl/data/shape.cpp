#include "shape.hpp"

#include <ostream>
#include <regex>
#include <sstream>

namespace prtcl {

void Shape::Save(ArchiveWriter &archive) const {
  archive.SaveSize(extents_.size());
  for (auto const extent : extents_)
    archive.SaveSize(extent);
}

void Shape::Load(ArchiveReader &archive) {
  extents_.resize(archive.LoadSize());
  for (auto &extent : extents_)
    extent = archive.LoadSize();
}

std::ostream &operator<<(std::ostream &o, Shape const &shape) {
  o << '[';
  for (size_t dim = 0; dim < shape.GetRank(); ++dim) {
    if (dim > 0)
      o << ", ";
    o << shape[dim];
  }
  o << ']';
  return o;
}

std::string Shape::ToString() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

Shape Shape::FromString(std::string_view input) {
  static std::regex const full_re{
      R"(\[[ \t]*[0-9]+([ \t]*,[ \t]*[0-9]+)*[ \t]*\])"};
  Shape result;
  if (std::regex_match(input.begin(), input.end(), full_re)) {
    static std::regex const part_re{R"([0-9]+)"};
    using iterator_type = std::regex_iterator<decltype(input)::const_iterator>;
    auto first = iterator_type{input.begin(), input.end(), part_re};
    auto last = iterator_type{};
    for (auto it = first; it != last; ++it) {
      result.extents_.push_back(std::stoul(it->str()));
    }
  }
  return result;
}

} // namespace prtcl

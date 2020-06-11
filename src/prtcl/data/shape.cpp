#include "shape.hpp"

#include <regex>
#include <sstream>

namespace prtcl {

std::string Shape::ToString() const {
  std::ostringstream ss;
  ss << '[';
  for (size_t dim = 0; dim < extents_.size(); ++dim) {
    if (dim > 0)
      ss << ", ";
    ss << extents_[dim];
  }
  ss << ']';
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

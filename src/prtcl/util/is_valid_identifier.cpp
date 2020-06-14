#include "is_valid_identifier.hpp"

#include <regex>

namespace prtcl {

bool IsValidIdentifier(std::string_view identifier) {
  static std::regex const valid_ident{R"([a-zA-Z][a-zA-Z0-9_]*)"};
  using std::begin, std::end;
  return std::regex_match(begin(identifier), end(identifier), valid_ident);
}

} // namespace prtcl

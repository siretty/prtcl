#include "component_type.hpp"

#include <ostream>
#include <type_traits>

namespace prtcl {

ComponentType const ComponentType::kInvalid = {CType::kInvalid};
ComponentType const ComponentType::kBoolean = {CType::kBoolean};
ComponentType const ComponentType::kSInt32 = {CType::kSInt32};
ComponentType const ComponentType::kSInt64 = {CType::kSInt64};
ComponentType const ComponentType::kFloat32 = {CType::kFloat32};
ComponentType const ComponentType::kFloat64 = {CType::kFloat64};

std::string_view ComponentType::ToStringView() const {
  switch (ctype_) {
  case CType::kBoolean:
    return "b";
  case CType::kSInt32:
    return "s32";
  case CType::kSInt64:
    return "s64";
  case CType::kFloat32:
    return "f32";
  case CType::kFloat64:
    return "f64";
  default:
    return "INVALID";
  }
}

std::ostream &operator<<(std::ostream &o, ComponentType const &ctype) {
  return (o << ctype.ToStringView());
}

ComponentType ComponentType::FromString(std::string_view input) {
  static std::regex const re{R"([a-z][0-9]*)"};
  std::match_results<decltype(input)::const_iterator> matches;
  auto Is32 = [](auto &s) {
    return s.size() == 3 and s[1] == '3' and s[2] == '2';
  };
  auto Is64 = [](auto &s) {
    return s.size() == 3 and s[1] == '6' and s[2] == '4';
  };
  if (std::regex_match(input.begin(), input.end(), matches, re)) {
    switch (input[0]) {
    case 'b':
      return input.size() == 1 ? kBoolean : kInvalid;
    case 's':
      return Is32(input) ? kSInt32 : (Is64(input) ? kSInt64 : kInvalid);
    case 'f':
      return Is32(input) ? kFloat32 : (Is64(input) ? kFloat64 : kInvalid);
    }
  }
  return kInvalid;
}

} // namespace prtcl

#ifndef PRTCL_SRC_PRTCL_DATA_COMPONENT_TYPE_HPP
#define PRTCL_SRC_PRTCL_DATA_COMPONENT_TYPE_HPP

#include <regex>
#include <string_view>
#include <variant>

#include <cstddef>

#include <boost/operators.hpp>

namespace prtcl {

class ComponentType : public boost::equality_comparable<ComponentType> {
public:
  ComponentType() = default;

public:
  bool IsValid() const { return ctype_ != CType::kInvalid; }

public:
  template <typename T>
  static ComponentType FromType();

public:
  std::string_view ToStringView() const;

  static ComponentType FromString(std::string_view input);

public:
  friend bool operator==(ComponentType const &lhs, ComponentType const &rhs) {
    return lhs.ctype_ == rhs.ctype_;
  }

public:
  static ComponentType const kInvalid;
  static ComponentType const kBoolean;
  static ComponentType const kSInt32;
  static ComponentType const kSInt64;
  static ComponentType const kFloat32;
  static ComponentType const kFloat64;

private:
  enum class CType {
    kInvalid = 0,
    kBoolean,
    kSInt32,
    kSInt64,
    kFloat32,
    kFloat64,
  };

  ComponentType(CType ctype) : ctype_{ctype} {}

private:
  CType ctype_ = CType::kInvalid;
};

template <typename T>
ComponentType MakeComponentType() {
  if constexpr (std::is_integral_v<T>) {
    if constexpr (std::is_same_v<T, bool>)
      return ComponentType::kBoolean;

    if constexpr (std::is_signed_v<T>) {
      if constexpr (sizeof(T) == 4)
        return ComponentType::kSInt32;
      if constexpr (sizeof(T) == 8)
        return ComponentType::kSInt64;
    }
  }

  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4)
      return ComponentType::kFloat32;
    if constexpr (sizeof(T) == 8)
      return ComponentType::kFloat64;
  }

  return {};
}

using ComponentVariant = std::variant<bool, int32_t, int64_t, float, double>;

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_COMPONENT_TYPE_HPP

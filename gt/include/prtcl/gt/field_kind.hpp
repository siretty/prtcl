#pragma once

#include <prtcl/core/invalid_enumerator_error.hpp>

#include <string_view>

namespace prtcl::gt {

enum class field_kind {
  global,
  uniform,
  varying,
};

inline constexpr std::string_view enumerator_name(field_kind kind_) {
  switch (kind_) {
  case field_kind::global:
    return "global";
  case field_kind::uniform:
    return "uniform";
  case field_kind::varying:
    return "varying";
  default:
    throw invalid_enumerator_error<field_kind>{kind_};
  }
}

} // namespace prtcl::gt

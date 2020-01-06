#pragma once

#include <prtcl/core/invalid_enumerator_error.hpp>

#include <string_view>

namespace prtcl::gt {

enum class index_kind {
  particle,
  neighbour,
};

inline constexpr std::string_view enumerator_name(index_kind type_) {
  switch (type_) {
  case index_kind::particle:
    return "particle";
  case index_kind::neighbour:
    return "neighbour";
  default:
    throw invalid_enumerator_error<index_kind>{type_};
  }
}

} // namespace prtcl::gt

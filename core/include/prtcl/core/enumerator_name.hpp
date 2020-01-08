#pragma once

#include "invalid_enumerator_error.hpp"

#include <string_view>

#define PRTCL_CORE_BEGIN_ENUMERATOR_NAME(ENUM_)                                \
  inline constexpr std::string_view enumerator_name(ENUM_ enum_) {             \
    using enum_type = ENUM_;                                                   \
    switch (enum_) {

#define PRTCL_CORE_ENUMERATOR_NAME(NAME_)                                      \
  case enum_type::NAME_:                                                       \
    return #NAME_;

#define PRTCL_CORE_CLOSE_ENUMERATOR_NAME()                                     \
  default:                                                                     \
    throw invalid_enumerator_error<enum_type>{enum_};                          \
    } /* switch */                                                             \
    } /* function */

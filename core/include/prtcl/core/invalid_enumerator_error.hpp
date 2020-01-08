#pragma once

#include <exception>
#include <type_traits>

namespace prtcl {

template <typename Enum_> struct invalid_enumerator_error : std::exception {
  static_assert(std::is_enum<Enum_>{}(), "");

  invalid_enumerator_error() noexcept = delete;

  invalid_enumerator_error(invalid_enumerator_error const &) noexcept = default;
  invalid_enumerator_error &
  operator=(invalid_enumerator_error const &) noexcept = default;

  invalid_enumerator_error(invalid_enumerator_error &&) noexcept = default;
  invalid_enumerator_error &
  operator=(invalid_enumerator_error &&) noexcept = default;

  invalid_enumerator_error(Enum_ enumerator_) noexcept
      : enumerator{enumerator_} {}

  ~invalid_enumerator_error() noexcept override = default;

  char const *what() const noexcept override {
    return "invalid enumerator value";
  }

  Enum_ enumerator;
};

} // namespace prtcl

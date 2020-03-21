#pragma once

namespace prtcl::core {

/// Helper type for visitors.
template <typename... Functors_> struct overloaded : Functors_... {
  using Functors_::operator()...;
};

/// Deduction guide for easy instanciation.
template <typename... Functors_>
overloaded(Functors_...)->overloaded<Functors_...>;

} // namespace prtcl::core

#ifndef PRTCL_DEFINE_TAG_OSTREAM_LSHIFT

#include <ostream>
#include <utility>

#define PRTCL_DEFINE_TAG_OPERATORS(NAME)                                       \
  friend std::ostream &operator<<(std::ostream &s, NAME) { return s << #NAME; }

#define PRTCL_DEFINE_TAG_COMPARISON(TRAIT)                                     \
  template <typename LHS, typename RHS>                                        \
  constexpr auto operator==(LHS &&, RHS &&)                                    \
      ->std::enable_if_t<TRAIT<meta::remove_cvref_t<LHS>>::value &&            \
                             TRAIT<meta::remove_cvref_t<RHS>>::value,          \
                         bool> {                                               \
    return std::is_same<meta::remove_cvref_t<LHS>,                             \
                        meta::remove_cvref_t<RHS>>::value;                     \
  }                                                                            \
                                                                               \
  template <typename LHS, typename RHS>                                        \
  constexpr auto operator!=(LHS &&lhs, RHS &&rhs)                              \
      ->std::enable_if_t<TRAIT<meta::remove_cvref_t<LHS>>::value &&            \
                             TRAIT<meta::remove_cvref_t<RHS>>::value,          \
                         bool> {                                               \
    return !(std::forward<LHS>(lhs) == std::forward<RHS>(rhs));                \
  }

#endif

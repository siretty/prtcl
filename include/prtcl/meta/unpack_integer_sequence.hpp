#pragma once

#include <utility>

namespace prtcl::detail {

template <typename IntegerSequence> struct integer_sequence_unpacker;

template <typename I, I... Is>
struct integer_sequence_unpacker<std::integer_sequence<I, Is...>> {
  template <typename F> constexpr auto operator()(F &&f) const {
    return std::forward<F>(f)(Is...);
  }
};

template <typename, typename> struct integer_sequences_unpacker;

template <typename I, I... I1s, I... I2s>
struct integer_sequences_unpacker<std::integer_sequence<I, I1s...>,
                                  std::integer_sequence<I, I2s...>> {
  template <typename F> constexpr auto operator()(F &&f) const {
    return std::forward<F>(f)(std::make_pair(I1s, I2s)...);
  }
};

} // namespace prtcl::detail

namespace prtcl::meta {

template <typename F, typename IntegerSequence>
constexpr auto unpack_integer_sequence(F &&f, IntegerSequence) {
  return ::prtcl::detail::integer_sequence_unpacker<IntegerSequence>{}(
      std::forward<F>(f));
}

template <typename F, typename IS1, typename IS2>
constexpr auto unpack_integer_sequences(F &&f, IS1, IS2) {
  return ::prtcl::detail::integer_sequences_unpacker<IS1, IS2>{}(
      std::forward<F>(f));
}

} // namespace prtcl::meta

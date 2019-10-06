#pragma once

namespace prtcl::detail {

template <typename, typename> struct ctor_call_expand_pack_impl;

template <typename Type, size_t... Ns>
struct ctor_call_expand_pack_impl<Type, std::index_sequence<Ns...>> {
  template <typename F, typename... Args>
  Type operator()(F f, Args &&... args) const {
    return Type{{f(Ns, std::forward<Args>(args)...)...}};
  }
};

template <typename Type, size_t N>
constexpr ctor_call_expand_pack_impl<Type, std::make_index_sequence<N>>
    ctor_call_expand_pack = {};

} // namespace prtcl::detail

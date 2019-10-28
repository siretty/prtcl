#pragma once

namespace prtcl::meta {

template <typename... Fs> struct overload : Fs... { using Fs::operator()...; };
template <typename... Fs> overload(Fs...)->overload<Fs...>;

} // namespace prtcl::meta

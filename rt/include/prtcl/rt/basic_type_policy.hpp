#pragma once

#include "common.hpp"

namespace prtcl::rt {

namespace detail {

// {{{ select_dtype_type_t

template <typename, nd_dtype> struct select_dtype_type;

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::real> {
  using type = typename TypePolicy_::real;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::integer> {
  using type = typename TypePolicy_::integer;
};

template <typename TypePolicy_>
struct select_dtype_type<TypePolicy_, nd_dtype::boolean> {
  using type = typename TypePolicy_::boolean;
};

template <typename TypePolicy_, nd_dtype DType_>
using select_dtype_type_t =
    typename select_dtype_type<TypePolicy_, DType_>::type;

// }}}

} // namespace detail

template <typename Real_, typename Integer_, typename Boolean_>
struct basic_type_policy {
  using real = Real_;
  using integer = Integer_;
  using boolean = Boolean_;

  template <nd_dtype DType_>
  using dtype_t = detail::select_dtype_type_t<basic_type_policy, DType_>;
};

using fib_type_policy = basic_type_policy<float, int, bool>;

} // namespace prtcl::rt

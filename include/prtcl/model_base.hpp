#pragma once

#include <prtcl/data/group.hpp>
#include <prtcl/data/ndfield.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <utility>

namespace prtcl {

struct model_base {
  template <typename T, size_t... Ns>
  using ndfield_ref_t = ::prtcl::data::ndfield_ref_t<T, Ns...>;

  template <typename T, size_t N> using scheme_t = ::prtcl::data::scheme<T, N>;

  template <typename T, size_t N> using group_t = ::prtcl::data::group<T, N>;
};

} // namespace prtcl

namespace prtcl_model {

using namespace prtcl::expr_literals;

} // namespace prtcl_model

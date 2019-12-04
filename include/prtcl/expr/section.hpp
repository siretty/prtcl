#pragma once

#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/transform/group_tag_terminal_value_xform.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <string>
#include <utility>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

struct section {
  std::string name;
};

template <typename... Exprs>
auto make_named_section(std::string name_, Exprs &&... exprs_) {
  return boost::yap::make_terminal(section{name_})(boost::yap::transform_strict(
      boost::yap::transform(std::forward<Exprs>(exprs_),
                            group_tag_terminal_value_xform{}),
      make_eq_xform<geq_xform>([](auto &&e) {
        return boost::yap::make_terminal(make_eq(std::forward<decltype(e)>(e)));
      }),
      foreach_particle_identity_xform{})...);
}

template <typename... Exprs> auto make_anonymous_section(Exprs &&... exprs_) {
  return make_named_section(std::string{""}, std::forward<Exprs>(exprs_)...);
}

} // namespace prtcl::expr

namespace prtcl::expr_language {

using ::prtcl::expr::make_anonymous_section;
using ::prtcl::expr::make_named_section;

} // namespace prtcl::expr_language

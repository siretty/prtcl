#pragma once

#include <prtcl/tag/call.hpp>

#include <boost/yap/yap.hpp>

/*
namespace prtcl::expr {

template <typename CallTag> struct call {
  static_assert(tag::is_call_v<CallTag>, "CallTag is invalid");

  using call_tag_type = CallTag;
  constexpr static call_tag_type call_tag = {};
};

} // namespace prtcl::expr
*/

namespace prtcl::expr_language {

constexpr boost::yap::expression<boost::yap::expr_kind::terminal,
                                 boost::hana::tuple<prtcl::tag::call::dot>>
    dot = {};

constexpr boost::yap::expression<boost::yap::expr_kind::terminal,
                                 boost::hana::tuple<prtcl::tag::call::norm>>
    norm = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::norm_squared>>
    norm_squared = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::normalized>>
    normalized = {};

constexpr boost::yap::expression<boost::yap::expr_kind::terminal,
                                 boost::hana::tuple<prtcl::tag::call::max>>
    max = {};

constexpr boost::yap::expression<boost::yap::expr_kind::terminal,
                                 boost::hana::tuple<prtcl::tag::call::min>>
    min = {};

constexpr boost::yap::expression<boost::yap::expr_kind::terminal,
                                 boost::hana::tuple<prtcl::tag::call::kernel>>
    kernel = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::kernel_gradient>>
    kernel_gradient = {};

} // namespace prtcl::expr_language

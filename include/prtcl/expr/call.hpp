#pragma once

#include <prtcl/tag/call.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::expr_language {

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal, boost::hana::tuple<prtcl::tag::call::dot>>
    dot = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal, boost::hana::tuple<prtcl::tag::call::norm>>
    norm = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::norm_squared>>
    norm_squared = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::normalized>>
    normalized = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal, boost::hana::tuple<prtcl::tag::call::max>>
    max = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal, boost::hana::tuple<prtcl::tag::call::min>>
    min = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::kernel>>
    kernel = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::kernel_gradient>>
    kernel_gradient = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::particle_count>>
    particle_count = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::neighbour_count>>
    neighbour_count = {};

constexpr boost::yap::expression<
    boost::yap::expr_kind::terminal,
    boost::hana::tuple<prtcl::tag::call::zero_vector>>
    zero_vector = {};

} // namespace prtcl::expr_language

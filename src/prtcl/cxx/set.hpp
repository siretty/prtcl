#ifndef PRTCL_SRC_PRTCL_CXX_SET_HPP
#define PRTCL_SRC_PRTCL_CXX_SET_HPP

#include <boost/container/flat_set.hpp>

namespace prtcl::cxx {

/// Flat std::set-like container with homogeneous lookup.
template <typename Value, typename Compare = std::less<Value>>
using hom_flat_set = boost::container::flat_set<Value, Compare>;

/// Flat std::set-like container with heterogeneous lookup.
template <typename Value, typename Compare = std::less<void>>
using het_flat_set = boost::container::flat_set<Value, Compare>;

} // namespace prtcl::cxx

#endif // PRTCL_SRC_PRTCL_CXX_SET_HPP

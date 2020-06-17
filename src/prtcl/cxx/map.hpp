#ifndef PRTCL_SRC_PRTCL_CXX_MAP_HPP
#define PRTCL_SRC_PRTCL_CXX_MAP_HPP

#include <boost/container/flat_map.hpp>

namespace prtcl::cxx {

/// Flat std::map-like container with homogeneous lookup.
template <typename Key, typename Value, typename Compare = std::less<Key>>
using hom_flat_map = boost::container::flat_map<Key, Value, Compare>;

/// Flat std::map-like container with heterogeneous lookup.
template <typename Key, typename Value, typename Compare = std::less<void>>
using het_flat_map = boost::container::flat_map<Key, Value, Compare>;

} // namespace prtcl::cxx

#endif // PRTCL_SRC_PRTCL_CXX_MAP_HPP

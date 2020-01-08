#pragma once

#include <initializer_list>
#include <string>

#include <boost/container/small_vector.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace prtcl::gt::ast {

using nd_shape = boost::container::small_vector<size_t, 2>;
using nd_index = boost::container::small_vector<size_t, 2>;

// class nd_shape {
// public:
//  size_t rank() const { return _shape.size(); }
//
//  auto extents() const { return boost::make_iterator_range(_shape); }
//
// public:
//  nd_shape() = default;
//
//  nd_shape(nd_shape const &) = default;
//  nd_shape &operator=(nd_shape const &) = default;
//
//  nd_shape(nd_shape &&) = default;
//  nd_shape &operator=(nd_shape &&) = default;
//
// public:
//  explicit nd_shape(size_t rank_) : _shape(rank_) {}
//
//  nd_shape(std::initializer_list<size_t> init_) : _shape{init_} {}
//
// private:
//  boost::container::small_vector<size_t, 2> _shape;
//};

} // namespace prtcl::gt

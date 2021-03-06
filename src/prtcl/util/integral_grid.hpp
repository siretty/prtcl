#pragma once

#include <array>
#include <numeric>

#include <cstddef>

#include <boost/iterator/iterator_facade.hpp>

namespace prtcl {

template <size_t N>
struct IntegralGridIterator
    : boost::iterator_facade<
          IntegralGridIterator<N>, std::array<size_t, N> const,
          boost::forward_traversal_tag> {
  std::array<size_t, N> current;
  std::array<size_t, N> extents;

private:
  friend class boost::iterator_core_access;

  void increment() {
    // increment the last index
    ++current.back();
    // overflow from back to front
    for (size_t n = 1; n <= N; ++n) {
      if (current[N - n] >= extents[N - n]) {
        if (N - n > 0) {
          current[N - n] = 0;
          ++current[N - n - 1];
        }
      }
    }
  }

  bool equal(IntegralGridIterator const &other) const {
    return current == other.current;
  }

  auto &dereference() const { return current; }
};

template <size_t N>
struct IntegralGrid {
  std::array<size_t, N> extents;

public:
  size_t size() const {
    return std::accumulate(
        extents.begin(), extents.end(), size_t{1}, std::multiplies<>{});
  }

public:
  auto begin() const {
    IntegralGridIterator<N> first;
    first.current.fill(0);
    first.extents = extents;
    return first;
  }

  auto end() const {
    IntegralGridIterator<N> last;
    last.current.fill(0);
    last.extents = extents;
    last.current[0] = extents[0];
    return last;
  }
};

} // namespace prtcl

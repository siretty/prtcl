#pragma once

#define SPHXX_ASSERT(...)
//#include <sphxx/core/assert.hpp>

#define SPHXX_INLINE
//#include <sphxx/core/attributes.hpp>

#include "../../math/constpow.hpp"
#include "../../math/morton_order.hpp"
#include "../../math/traits/host_math_traits.hpp"

#include <array>
#include <iterator>
#include <utility>
#include <vector>

#include <cstddef>
#include <cstdint>

#include <iostream>

#include <boost/sort/sort.hpp>

namespace prtcl {

// implementation detail {{{
namespace detail {

template <size_t N> using grid_index_t = std::array<int32_t, N>;

template <size_t N> struct sparse_adjacent_cell_offsets;

template <> struct sparse_adjacent_cell_offsets<1> {
  constexpr static std::array<grid_index_t<1>, 1> value = {{
      {1},
  }};
};

template <> struct sparse_adjacent_cell_offsets<2> {
  constexpr static std::array<grid_index_t<2>, 4> value = {{
      {1, 0},
      {0, 1},
      {1, 1},
      {1, -1},
  }};
};

template <> struct sparse_adjacent_cell_offsets<3> {
  constexpr static std::array<grid_index_t<3>, 13> value = {{
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
      {1, 1, 0},
      {1, 0, 1},
      {0, 1, 1},
      {1, 1, 1},
      {0, -1, 1},
      {1, 0, -1},
      {1, 1, -1},
      {1, -1, -1},
      {1, -1, 0},
      {1, -1, 1},
  }};
};

template <size_t N>
constexpr auto sparse_adjacent_cell_offsets_v =
    sparse_adjacent_cell_offsets<N>::value;

} // namespace detail
// }}}

template <typename T, size_t N, size_t GroupBits = 4, size_t IndexBits = 28>
class grouped_uniform_grid {
  static_assert(1 <= N && N <= 3);

  using math_traits = host_math_traits<T, N>;

public:
  using scalar_type = typename math_traits::scalar_type;
  static constexpr size_t vector_extent = math_traits::vector_extent;

private:
  // implementation member types {{{

  enum class sorted_index : int32_t { invalid = -1 };
  enum class cell_index : int32_t { invalid = -1 };
  using grid_index = detail::grid_index_t<N>;

  // }}}

public:
  // member types {{{

  enum class raw_group : size_t {};
  static constexpr size_t max_raw_group = constpow(2, GroupBits);

  enum class raw_index : size_t {};
  static constexpr size_t max_raw_index = constpow(2, IndexBits);

  struct raw_grouped_index {
    size_t get_group() const { return static_cast<size_t>(group); }
    size_t get_index() const { return static_cast<size_t>(index); }

    raw_group group : GroupBits;
    raw_index index : IndexBits;
  };

  // }}}

public:
  grouped_uniform_grid(scalar_type radius = 1) : radius_{radius} {}

public:
  scalar_type get_radius() const { return radius_; }

  void set_radius(scalar_type radius) { radius_ = radius; }

public:
  // update(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update(GroupedVectorData const &x) {
    SPHXX_ASSERT(N == get_vector_extent(x));

    update_sorted_to_raw(x);
    update_sorted_to_cell(x);
    update_cell_to_grid_and_sorted_range(x);
    update_cell_to_adjacent_cells(x);
  }

private:
  // update_sorted_to_raw(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_sorted_to_raw(GroupedVectorData const &x) {
    SPHXX_ASSERT(get_group_count(x) < max_raw_group);

    bool reset_sorted_to_raw_flag = false;

    // resize according to the current number of groups
    raw_to_sorted_.resize(get_group_count(x));

    // new number of sorted indices
    size_t new_sorted_index_count = 0;
    for (size_t group = 0; group < get_group_count(x); ++group) {
      // old number of raw indices in group
      size_t const old_raw_index_count = raw_to_sorted_[group].size();

      // new number of raw indices in group
      size_t const new_raw_index_count =
          get_element_count(get_group_ref(x, group));
      SPHXX_ASSERT(new_raw_index_count < max_raw_index);

      // resize according to the current number of raw indices in group
      raw_to_sorted_[group].resize(new_raw_index_count);

      // if the number of raw indices in _any_ group changed, flag for reset
      if (old_raw_index_count != new_raw_index_count)
        reset_sorted_to_raw_flag = true;

      // accumulate the overall number of raw indices
      new_sorted_index_count += new_raw_index_count;
    }

    if (reset_sorted_to_raw_flag) {
      // resize according to the new number of sorted indices
      sorted_to_raw_.resize(new_sorted_index_count);

      // reset sorted_to_raw_ to a valid, but unsorted permutation
      size_t n_s = 0;
      for (size_t n_g = 0; n_g < raw_to_sorted_.size(); ++n_g) {
        for (size_t n_r = 0; n_r < raw_to_sorted_[n_g].size(); ++n_r) {
          sorted_to_raw_[n_s] = raw_grouped_index{static_cast<raw_group>(n_g),
                                                  static_cast<raw_index>(n_r)};
          ++n_s;
        }
      }
    }

    // sort sorted_to_raw_ using the Morton order
    boost::sort::block_indirect_sort(
        sorted_to_raw_.begin(), sorted_to_raw_.end(),
        [this, &x](auto i_gr, auto j_gr) {
          return this->compate_raw_indices(x, i_gr, j_gr);
          // return morton_order(this->compute_grid_index(x, i_gr),
          //                    this->compute_grid_index(x, j_gr));
        });

    // update raw_to_sorted_ with the sorted indices
#pragma omp parallel for
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto const &i_gr = sorted_to_raw_[n_s];
      raw_to_sorted_[i_gr.get_group()][i_gr.get_index()] =
          static_cast<sorted_index>(n_s);
    }
  }

  // }}}

  // update_sorted_to_cell(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_sorted_to_cell(GroupedVectorData const &x) {
    // resize to match the current number of sorted indices
    sorted_to_cell_.resize(sorted_to_raw_.size());

    if (0 == sorted_to_cell_.size())
      return;

    // count the number of used grid cells
    size_t used_cell_count = 1;

    // the first cell index is always 0
    sorted_to_cell_[0] = static_cast<cell_index>(0);

#pragma omp parallel for reduction(+ : used_cell_count)
    for (size_t cur_n_s = 1; cur_n_s < sorted_to_cell_.size(); ++cur_n_s) {
      auto const &cur_i_gr = sorted_to_raw_[cur_n_s];
      auto const cur_gi = compute_grid_index(x, cur_i_gr);

      size_t prv_n_s = cur_n_s - 1;
      auto const &prv_i_gr = sorted_to_raw_[prv_n_s];
      auto const prv_gi = compute_grid_index(x, prv_i_gr);

      if (cur_gi == prv_gi) {
        // 0 if the previous and the current sorted index refer to the same cell
        sorted_to_cell_[cur_n_s] = static_cast<cell_index>(0);
      } else {
        // 1 if the previous and the current sorted index refer to different
        // cells
        sorted_to_cell_[cur_n_s] = static_cast<cell_index>(1);
        used_cell_count += 1;
      }
    }

    // resize the cell_to_... arrays
    cell_to_sorted_range_.resize(used_cell_count);
    cell_to_grid_.resize(used_cell_count);
    cell_to_adjacent_cells_.resize(used_cell_count);
  }

  // }}}

  // update_cell_to_grid_and_sorted_range(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_cell_to_grid_and_sorted_range(GroupedVectorData const &x) {
    if (0 == cell_to_sorted_range_.size())
      return;

      // invalidate all cell_to_... arrays
#pragma omp parallel for
    for (size_t n_c = 0; n_c < cell_to_grid_.size(); ++n_c) {
#if !defined(NDEBUG)
      cell_to_sorted_range_[n_c] =
          std::make_pair(sorted_index::invalid, sorted_index::invalid);
      cell_to_grid_[n_c].fill(std::numeric_limits<int32_t>::min());
#endif // !defined(NDEBUG)
      cell_to_adjacent_cells_[n_c].fill(cell_index::invalid);
    }

    // first cell
    cell_to_sorted_range_[0].first = static_cast<sorted_index>(0);
    cell_to_grid_[0] = compute_grid_index(x, sorted_to_raw_[0]);

    for (size_t cur_n_s = 1; cur_n_s < sorted_to_raw_.size(); ++cur_n_s) {
      auto const value = static_cast<size_t>(sorted_to_cell_[cur_n_s]);

      // cumulative sum to compute the actual cell index
      size_t n_c = static_cast<size_t>(sorted_to_cell_[cur_n_s - 1]) + value;
      sorted_to_cell_[cur_n_s] = static_cast<cell_index>(n_c);

      // update the cell_to_... for the first sorted index in the cell
      if (value == 1) {
        auto const cur_i_s = static_cast<sorted_index>(cur_n_s);

        // update the cell_to_sorted_range for the previous and current cell
        cell_to_sorted_range_[n_c - 1].second = cur_i_s;
        cell_to_sorted_range_[n_c].first = cur_i_s;

        // update the grid index of the cell
        cell_to_grid_[n_c] = compute_grid_index(x, sorted_to_raw_[cur_n_s]);
      }
    }

    // last cell
    cell_to_sorted_range_[cell_to_sorted_range_.size() - 1].second =
        static_cast<sorted_index>(sorted_to_raw_.size());
  }

  // }}}

  // update_cell_to_adjacent_cells(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_cell_to_adjacent_cells(GroupedVectorData const &x) {
#pragma omp parallel for
    for (size_t n_c = 0; n_c < cell_to_grid_.size(); ++n_c) {
      // enumerate all sparse adjacent cell offsets
      constexpr auto sparse_offsets = detail::sparse_adjacent_cell_offsets_v<N>;
      for (size_t sparse_offset_index = 0;
           sparse_offset_index < sparse_offsets.size(); ++sparse_offset_index) {
        grid_index adjacent_grid_index = sparse_offsets[sparse_offset_index];

        // compute the grid index of the adjacent cell
        for (size_t i = 0; i < N; ++i)
          adjacent_grid_index[i] += cell_to_grid_[n_c][i];

        if (cell_index j_c = find_cell(adjacent_grid_index);
            j_c != cell_index::invalid) {
          // update the adjacent cells of n_c
          cell_to_adjacent_cells_[n_c][sparse_offset_index] = j_c;

          // update the adjacent cells of j_c
          cell_to_adjacent_cells_[static_cast<size_t>(j_c)]
                                 [constpow(3, N) - 2 - sparse_offset_index] =
                                     static_cast<cell_index>(n_c);
        }
      }
    }
  }

  // }}}

  // find_cell(grid_index) -> cell_index {{{

  SPHXX_INLINE cell_index find_cell(grid_index const &gi) {
    return std::as_const(*this).find_cell(gi);
  }

  SPHXX_INLINE cell_index find_cell(grid_index const &gi) const {
    auto const first = cell_to_grid_.begin();
    auto const last = cell_to_grid_.end();
    auto const it =
        std::lower_bound(first, last, gi, [](auto const &lhs, auto const &rhs) {
          return morton_order(lhs, rhs);
        });

    if (it == last || *it != gi)
      return cell_index::invalid;
    else
      return static_cast<cell_index>(std::distance(first, it));
  }

  // }}}

  template <typename GroupedVectorData>
  SPHXX_INLINE bool compate_raw_indices(GroupedVectorData const &x,
                                        raw_grouped_index const &lhs,
                                        raw_grouped_index const &rhs) {
    auto lhs_gi = this->compute_grid_index(x, lhs);
    auto rhs_gi = this->compute_grid_index(x, rhs);
    return morton_order(lhs_gi, rhs_gi);
    // if (morton_order(lhs_gi, rhs_gi))
    //  return true;
    // if (lhs_gi == rhs_gi) {
    //  // if (lhs.get_group() == rhs.get_group()) {
    //  //  return lhs.get_index() < rhs.get_index();
    //  //}
    //  return lhs.get_group() < rhs.get_group();
    //}
    // return false;
  }

  // }}}

public:
  // compute_group_permutation(group, perm) {{{

  template <typename OutputIt>
  void compute_group_permutation(size_t group, OutputIt it) const {
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto i_gr = sorted_to_raw_[n_s];
      if (i_gr.get_group() == group) {
        *(it++) =
            static_cast<typename std::iterator_traits<OutputIt>::value_type>(
                i_gr.get_index());
      }
    }
  }

  /* debug version of compute_group_permutation {{{

  template <typename Perm>
  void compute_group_permutation(size_t group, Perm &perm) const {
    size_t n_p = 0;
    // std::cerr << std::endl;
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto i_gr = sorted_to_raw_[n_s];
      // auto i_c = sorted_to_cell_[n_s];
      // auto gi = cell_to_grid_[static_cast<size_t>(i_c)];
      // auto v =
      //    get_element_ref(get_group_ref(x, i_gr.get_group()),
      //    i_gr.get_index());
      if (i_gr.get_group() == group) {
        perm[n_p++] = static_cast<typename Perm::value_type>(i_gr.get_index());
        // std::cerr << "s2r[" << n_s << "] = (" << i_gr.get_group() << ", "
        //          << i_gr.get_index() << ") c = " << static_cast<size_t>(i_c)
        //          << " gi = (" << gi[0] << " " << gi[1] << ")"
        //          << " x = (" << v[0] << " " << v[1] << ")" << std::endl;
      }
    }
    // std::cerr << std::endl;
  }

  }}} */

  // }}}

public:
  // neighbours(group, index, x, fn) {{{

  template <typename GroupedVectorData, typename Fn>
  void neighbours(size_t group, size_t index, GroupedVectorData const &x,
                  Fn fn) const {
    SPHXX_ASSERT(N == get_vector_extent(x));

    scalar_type radius_squared = constpow(radius_, 2);
    potential_neighbours(
        group, index,
        [group, index, radius_squared, &fn, &x](raw_grouped_index const &i_gr) {
          auto const distance = math_traits::norm(
              get_element_ref(get_group_ref(x, group), index) -
              get_element_ref(get_group_ref(x, i_gr.get_group()),
                              i_gr.get_index()));
          if (distance < radius_squared) {
            fn(i_gr);
          }
        });
  }

  // }}}

public:
  // potential_neighbours(group, index, fn) {{{

  template <typename Fn>
  void potential_neighbours(size_t group, size_t index, Fn fn) const {
    SPHXX_ASSERT(group < max_raw_group);
    SPHXX_ASSERT(index < max_raw_index);

    raw_grouped_index i_gr{static_cast<raw_group>(group),
                           static_cast<raw_index>(index)};

    sorted_index i_s = raw_to_sorted(i_gr);
    cell_index i_c = sorted_to_cell(i_s);

    // lambda for_cell(cell_index) {{{
    auto for_cell = [this, &fn](cell_index const &j_c) {
      if (j_c == cell_index::invalid)
        return;

      auto const &range_s = cell_to_sorted_range(j_c);
      SPHXX_ASSERT(range_s.first != sorted_index::invalid);
      SPHXX_ASSERT(range_s.second != sorted_index::invalid);
      for (size_t i = static_cast<size_t>(range_s.first);
           i < static_cast<size_t>(range_s.second); ++i)
        fn(sorted_to_raw(static_cast<sorted_index>(i)));
    };
    // }}}

    for_cell(i_c);

    for (cell_index j_c : cell_to_adjacent_cells(i_c))
      for_cell(j_c);
  }

  // }}}

private:
  // raw_to_sorted : G × R -> S {{{

  auto const &raw_to_sorted(raw_grouped_index const &i_gr) const {
    SPHXX_ASSERT(i_gr.get_group() < max_raw_group);
    SPHXX_ASSERT(i_gr.get_index() < max_raw_index);
    return raw_to_sorted_[static_cast<size_t>(i_gr.group)]
                         [static_cast<size_t>(i_gr.index)];
  }

  std::vector<std::vector<sorted_index>> raw_to_sorted_;

  // }}}

  // sorted_to_raw : S -> G × R {{{

  auto const &sorted_to_raw(sorted_index const &i_s) const {
    SPHXX_ASSERT(i_s != sorted_index::invalid);
    return sorted_to_raw_[static_cast<size_t>(i_s)];
  }

  std::vector<raw_grouped_index> sorted_to_raw_;

  // }}}

  // sorted_to_cell : S -> C {{{

  auto const &sorted_to_cell(sorted_index const &i_s) const {
    SPHXX_ASSERT(i_s != sorted_index::invalid);
    return sorted_to_cell_[static_cast<size_t>(i_s)];
  }

  std::vector<cell_index> sorted_to_cell_;

  // }}}

  // cell_to_sorted_range : C -> S^2 {{{

  auto const &cell_to_sorted_range(cell_index const &i_c) const {
    SPHXX_ASSERT(i_c != cell_index::invalid);
    return cell_to_sorted_range_[static_cast<size_t>(i_c)];
  }

  std::vector<std::pair<sorted_index, sorted_index>> cell_to_sorted_range_;

  // }}}

  // cell_to_adjacent_cells : C -> C^(3^d - 1) {{{

  auto const &cell_to_adjacent_cells(cell_index const &i_c) const {
    SPHXX_ASSERT(i_c != cell_index::invalid);
    return cell_to_adjacent_cells_[static_cast<size_t>(i_c)];
  }

  std::vector<std::array<cell_index, constpow(3, N) - 1>>
      cell_to_adjacent_cells_;

  // }}}

  // cell_to_grid_index : C -> Z^d {{{

  auto const &cell_to_grid_index(cell_index const &i_c) const {
    SPHXX_ASSERT(i_c != cell_index::invalid);
    return cell_to_grid_[static_cast<size_t>(i_c)];
  }

  std::vector<grid_index> cell_to_grid_;

  // }}}

  // compute_grid_index(...) -> ... {{{

  template <typename GroupedVectorData>
  grid_index compute_grid_index(GroupedVectorData const &x,
                                raw_grouped_index i_gr) {
    grid_index result;
    using single_grid_index = typename grid_index::value_type;

    for (size_t axis = 0; axis < result.size(); ++axis) {
      result[axis] = static_cast<single_grid_index>(std::floor(
          get_element_ref(get_group_ref(x, i_gr.get_group()),
                          i_gr.get_index())[static_cast<Eigen::Index>(axis)] /
          radius_));
    }

    return result;
  }

  // }}}

private:
  scalar_type radius_;
}; // namespace sphxx

} // namespace prtcl

// NOTE:
//    The public interface of grouped_uniform_grid is inspired by
//    https://github.com/InteractiveComputerGraphics/CompactNSearch
//    Specifically the type CompactNSearch::NeighbourhoodSearch.
//    The algorithms used internally are different though.

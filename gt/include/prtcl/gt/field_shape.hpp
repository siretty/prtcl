#pragma once

#include <array>
#include <limits>
#include <ostream>

#include <cstddef>

namespace prtcl::gt {

struct scalar_shape_tag {};
struct vector_shape_tag {};
struct matrix_shape_tag {};

/*

template <size_t Rank_> class field_shape {
public:
  static constexpr size_t rank() { return Rank_; }

public:
  template <size_t OtherRank_>
  constexpr bool operator==(field_shape<OtherRank_> rhs_) const {
    if constexpr (Rank_ == OtherRank_)
      return _extents == rhs_._extents;
    else
      return false;
  }

  template <size_t OtherRank_>
  constexpr bool operator!=(field_shape<OtherRank_> rhs_) const {
    return not(*this == rhs_);
  }

private:
  friend std::ostream &operator<<(std::ostream &o_, field_shape const &s_) {
    o_ << "field_shape<" << s_.rank() << ">{"
       << "}";
    // using namespace boost::hana::literals;
    // o_ << "field_shape<";
    // if constexpr (sizeof...(Extents_) >= 1)
    //  o_ << boost::hana::make_tuple(Extents_...)[0_c];
    // o_ << ">";
  }

public:
  field_shape() = default;

  field_shape(field_shape const &) = default;
  field_shape &operator=(field_shape const &) = default;

  field_shape(field_shape &&) = default;
  field_shape &operator=(field_shape &&) = default;

  explicit field_shape(size_t value_) { _extents.fill(value_); }

  explicit field_shape(std::array<size_t, Rank_> const &extents_)
      : _extents{extents_} {}

private:
  std::array<size_t, Rank_> _extents;
};
 
 */

} // namespace prtcl::gt

#pragma once

#include <prtcl/gt/field_shape.hpp>
#include <prtcl/gt/field_type.hpp>

#include <array>
#include <ostream>
#include <string>
#include <utility>

#include <cstddef>

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class constant {
public:
  using shape_type = ::boost::container::small_vector<size_t, 2>;
  using content_type = std::string;

public:
  constexpr auto type() const { return _type; }

  size_t rank() const { return _shape.size(); }

  auto shape() const { return ::boost::make_iterator_range(_shape); }

  auto content() const { return _content; }

public:
  constant() = delete;

  constant(constant const &) = default;
  constant &operator=(constant const &) = default;

  constant(constant &&) = default;
  constant &operator=(constant &&) = default;

  template <typename Content_>
  constant(
      field_type type_, std::initializer_list<size_t> shape_,
      Content_ &&content_)
      : _type{type_}, _shape{shape_.begin(), shape_.end()},
        _content{std::forward<Content_>(content_)} {
    // TODO: validate type-shape combination
    // TODO: validate name
  }

  template <typename Content_>
  constant(field_type type_, scalar_shape_tag, Content_ &&content_)
      : constant{type_, std::initializer_list<size_t>{},
                 std::forward<Content_>(content_)} {}

  template <typename Content_>
  constant(field_type type_, vector_shape_tag, Content_ &&content_)
      : constant{type_, {0}, std::forward<Content_>(content_)} {}

  template <typename Content_>
  constant(field_type type_, matrix_shape_tag, Content_ &&content_)
      : constant{type_, {0, 0}, std::forward<Content_>(content_)} {}

public:
  bool operator==(constant rhs_) const {
    return _type == rhs_.type() and _shape == rhs_.shape();
  }

  bool operator!=(constant rhs_) const { return not(*this == rhs_); }

private:
  field_type _type;
  shape_type _shape;
  content_type _content;

  friend std::ostream &operator<<(std::ostream &o_, constant const &c_) {
    o_ << "constant{" << enumerator_name(c_.type()) << ", {"
       << boost::algorithm::join(
              c_.shape() | boost::adaptors::transformed(
                               [](auto v_) { return std::to_string(v_); }),
              ", ")
       << "}, \"" << c_.content() << "\"}";
    return o_;
  }
};

// TODO: integrate numeric constants

} // namespace prtcl::gt

#pragma once

#include <prtcl/gt/field_kind.hpp>
#include <prtcl/gt/field_shape.hpp>
#include <prtcl/gt/field_type.hpp>

#include <array>
#include <initializer_list>
#include <ostream>
#include <string>
#include <utility>

#include <cstddef>

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/operators.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class field : public ::boost::totally_ordered<field> {
public:
  using shape_type = ::boost::container::small_vector<size_t, 2>;
  using name_type = std::string;

public:
  constexpr auto kind() const { return _kind; }

  constexpr auto type() const { return _type; }

  size_t rank() const { return _shape.size(); }

  auto shape() const { return ::boost::make_iterator_range(_shape); }

  auto name() const { return _name; }

public:
  field() = delete;

  field(field const &) = default;
  field &operator=(field const &) = default;

  field(field &&) = default;
  field &operator=(field &&) = default;

  template <typename Name_>
  explicit field(
      field_kind kind_, field_type type_, std::initializer_list<size_t> shape_,
      Name_ &&name_)
      : _kind{kind_}, _type{type_}, _shape{shape_.begin(), shape_.end()},
        _name{std::forward<Name_>(name_)} {
    // TODO: validate kind-type-shape combination
    // TODO: validate name
  }

  template <typename Name_>
  explicit field(
      field_kind kind_, field_type type_, scalar_shape_tag, Name_ &&name_)
      : field{kind_, type_, std::initializer_list<size_t>{},
              std::forward<Name_>(name_)} {}

  template <typename Name_>
  explicit field(
      field_kind kind_, field_type type_, vector_shape_tag, Name_ &&name_)
      : field{kind_, type_, {0}, std::forward<Name_>(name_)} {}

  template <typename Name_>
  explicit field(
      field_kind kind_, field_type type_, matrix_shape_tag, Name_ &&name_)
      : field{kind_, type_, {0, 0}, std::forward<Name_>(name_)} {}

public:
  bool operator==(field rhs_) const {
    return _kind == rhs_.kind() and _type == rhs_.type() and
           _shape == rhs_.shape() and _name == rhs_.name();
  }

  bool operator<(field rhs_) const {
    auto lhs_kt = std::make_tuple(kind(), type());
    auto rhs_kt = std::make_tuple(rhs_.kind(), rhs_.type());
    if (lhs_kt < rhs_kt)
      return true;
    else if (lhs_kt > rhs_kt)
      return false;
    else {
      auto lhs_sh = shape();
      auto rhs_sh = rhs_.shape();
      if (lhs_sh < rhs_sh)
        return true;
      else if (lhs_sh > rhs_sh)
        return false;
      else
        return name() < rhs_.name();
    }
  }

private:
  field_kind _kind;
  field_type _type;
  shape_type _shape;
  name_type _name;

  friend std::ostream &operator<<(std::ostream &o_, field const &f_) {
    o_ << "field{" << enumerator_name(f_.kind()) << ", "
       << enumerator_name(f_.type()) << ", {"
       << boost::algorithm::join(
              f_.shape() | boost::adaptors::transformed(
                               [](auto v_) { return std::to_string(v_); }),
              ", ")
       << "}, \"" << f_.name() << "\"}";
    return o_;
  }
};

} // namespace prtcl::gt

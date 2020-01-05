#pragma once

#include <prtcl/core/shape.hpp>
#include <prtcl/gt/field_kind.hpp>
#include <prtcl/gt/field_type.hpp>

#include <ostream>
#include <string>
#include <utility>

#include <cstddef>

#include <boost/yap/yap.hpp>

namespace prtcl::gt {

template <field_kind Kind_, field_type Type_, typename Shape_> class field {
  static_assert(is_shape_v<Shape_>, "");

public:
  static constexpr field_kind kind() { return Kind_; }

  static constexpr field_type type() { return Type_; }

  static constexpr auto shape() { return Shape_{}; }

public:
  auto name() const { return _name; }

public:
  field() = delete;

  field(field const &) = default;
  field &operator=(field const &) = default;

  field(field &&) = default;
  field &operator=(field &&) = default;

  field(std::string_view name_) : _name{name_} {}

private:
  std::string _name;

  friend std::ostream &operator<<(std::ostream &o_, field const &f_) {
    o_ << "field<" << enumerator_name(f_.kind()) << ", "
       << enumerator_name(f_.type()) << ", " << f_.shape() << ">{" << f_.name()
       << "}";
  }
};

template <field_kind Kind_, field_type Type_>
using scalar_field = field<Kind_, Type_, shape<>>;

template <field_kind Kind_, field_type Type_, size_t E0_ = 0>
using vector_field = field<Kind_, Type_, shape<E0_>>;

template <field_kind Kind_, field_type Type_, size_t E0_ = 0, size_t E1_ = 0>
using matrix_field = field<Kind_, Type_, shape<E0_, E1_>>;

} // namespace prtcl::gt

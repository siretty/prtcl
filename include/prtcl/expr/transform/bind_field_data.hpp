#pragma once

#include "../../data/group_data.hpp"
#include "../grammar/field.hpp"
#include "../grammar/index.hpp"

#include "../terminal/varying_scalar.hpp"

#include "../transform/transform_fields.hpp"

#include <iostream>

#include <cstddef>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

struct BindFieldTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  template <typename This, typename Name, typename Index, typename Data>
  struct result<This(varying_scalar<Name> const &, Index &, Data &)> {
    using type =
        varying_scalar_term<typename Data::group_data_type::scalars_type>;
  };

public:
  template <typename Name, typename Data>
  varying_scalar_term<typename Data::group_data_type::scalars_type>
  operator()(varying_scalar<Name> const &field, active_index const &,
             Data const &data) const {
    std::cout << "binding data of active varying_scalar " << field.value << std::endl;
    return {*data.active.get_varying_scalar(field.value)};
  }

  template <typename Name, typename Data>
  varying_scalar_term<typename Data::group_data_type::scalars_type>
  operator()(varying_scalar<Name> const &field, neighbour_index const &,
             Data const &data) const {
    std::cout << "binding data of neighbour varying_scalar " << field.value << std::endl;
    return {*data.neighbour.get_varying_scalar(field.value)};
  }

public:
  template <typename This, typename Name, typename Index, typename Data>
  struct result<This(varying_vector<Name> const &, Index &, Data &)> {
    using type =
        varying_vector_term<typename Data::group_data_type::vectors_type>;
  };

public:
  template <typename Name, typename Data>
  varying_vector_term<typename Data::group_data_type::vectors_type>
  operator()(varying_vector<Name> const &field, active_index const &,
             Data const &data) const {
    return {*data.active.get_varying_vector(field.value)};
  }

  template <typename Name, typename Data>
  varying_vector_term<typename Data::group_data_type::vectors_type>
  operator()(varying_vector<Name> const &field, neighbour_index const &,
             Data const &data) const {
    return {*data.neighbour.get_varying_vector(field.value)};
  }
};

} // namespace detail

template <typename E, typename T, size_t N>
auto bind_field_data(E const &e, group_data<T, N> &active,
                     group_data<T, N> &neighbour) {
  struct {
    using group_data_type = group_data<T, N>;

    group_data_type &active;
    group_data_type &neighbour;
  } groups{active, neighbour};

  detail::TransformFields<detail::BindFieldTransform> transform;
  return boost::proto::deep_copy(transform(e, 0, groups));
}

} // namespace prtcl::expr

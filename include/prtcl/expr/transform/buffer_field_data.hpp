#pragma once

#include "../../data/group_data.hpp"
#include "../grammar/field.hpp"
#include "../grammar/index.hpp"

#include "../terminal/varying_scalar.hpp"

#include "../transform/transform_fields.hpp"

#include <type_traits>

#include <cstddef>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

struct BufferFieldTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  //template <typename This, typename FieldData, typename Index, typename Data>
  //struct result<This(varying_scalar<FieldData> &, Index &, Data &)> {
  //  using type =
  //      varying_scalar_term<decltype(get_buffer(std::declval<FieldData &>()))>;
  //};

  template <typename This, typename FieldData, typename Index, typename Data>
  struct result<This(varying_scalar<FieldData> const &, Index &, Data &)> {
    using type =
        varying_scalar_term<decltype(get_buffer(std::declval<FieldData &>()))>;
  };

public:
  template <typename FieldData, typename Data>
  varying_scalar_term<decltype(get_buffer(std::declval<FieldData &>()))>
  operator()(varying_scalar<FieldData> const &field, active_index const &,
             Data const &) const {
    return {get_buffer(field.value)};
  }

  template <typename FieldData, typename Data>
  varying_scalar_term<decltype(get_buffer(std::declval<FieldData &>()))>
  operator()(varying_scalar<FieldData> const &field, neighbour_index const &,
             Data const &) const {
    return {get_buffer(field.value)};
  }

  /*
public:
  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_vector<FieldData> &, Index &, Data &,
                     Args... args)> {
    using type = varying_vector_term<decltype(
        get_buffer(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_vector<FieldData> const &, Index &, Data &,
                     Args... args)> {
    using type = varying_vector_term<decltype(
        get_buffer(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

public:
  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_vector<FieldData> const &field, active_index const &,
                  Data const &, Args &&... args) const {
    return varying_vector_term<decltype(
        get_buffer(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_buffer(field.value, std::forward<Args>(args)...)};
  }

  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_vector<FieldData> const &field, neighbour_index const
&, Data const &, Args &&... args) const { return varying_vector_term<decltype(
        get_buffer(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_buffer(field.value, std::forward<Args>(args)...)};
  }
  */
};

} // namespace detail

template <typename E> auto buffer_field_data(E const &e) {
  detail::TransformFields<detail::BufferFieldTransform> transform;
  return transform(e, 0, 0);
}

} // namespace prtcl::expr

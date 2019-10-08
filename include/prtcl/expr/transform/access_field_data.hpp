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

struct RWAccessFieldTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> &, Index &, Data &,
                     Args... args)> {
    using type = varying_scalar_term<decltype(
        get_rw_access(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> const &, Index &, Data &,
                     Args... args)> {
    using type = varying_scalar_term<decltype(
        get_rw_access(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

public:
  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field_, active_index const &,
                  Data const &, Args &&... args) const {
    varying_scalar<FieldData> field{field_};
    return varying_scalar_term<decltype(
        get_rw_access(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_rw_access(field.value, std::forward<Args>(args)...)};
  }

  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field_,
                  neighbour_index const &, Data const &,
                  Args &&... args) const {
    varying_scalar<FieldData> field{field_};
    return varying_scalar_term<decltype(
        get_rw_access(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_rw_access(field.value, std::forward<Args>(args)...)};
  }
};

struct ROAccessFieldTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> &, Index &, Data &,
                     Args... args)> {
    using type = varying_scalar_term<decltype(
        get_ro_access(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> const &, Index &, Data &,
                     Args... args)> {
    using type = varying_scalar_term<decltype(
        get_ro_access(std::declval<FieldData &>(), std::declval<Args>()...))>;
  };

public:
  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field_, active_index const &,
                  Data const &, Args &&... args) const {
    varying_scalar<FieldData> field{field_};
    return varying_scalar_term<decltype(
        get_ro_access(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_ro_access(field.value, std::forward<Args>(args)...)};
  }

  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field_,
                  neighbour_index const &, Data const &,
                  Args &&... args) const {
    varying_scalar<FieldData> field{field_};
    return varying_scalar_term<decltype(
        get_ro_access(std::declval<FieldData &>(), std::declval<Args>()...))>{
        get_ro_access(field.value, std::forward<Args>(args)...)};
  }
};

} // namespace detail

template <typename E> auto access_field_data(E const &e) {
  detail::AssignTransformFields<detail::RWAccessFieldTransform,
                                detail::ROAccessFieldTransform>
      transform;
  return transform(e, 0, 0);
}

} // namespace prtcl::expr

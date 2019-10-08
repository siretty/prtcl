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

struct EvaluateAssignTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  template <typename This, typename FieldData, typename Index, typename Value,
            typename Data>
  struct result<This(varying_scalar<FieldData> &, Index, Value, Data)> {
    using type = typename FieldData::value_type;
  };

  template <typename This, typename FieldData, typename Index, typename Value,
            typename Data>
  struct result<This(varying_scalar<FieldData> const &, Index, Value, Data)> {
    using type = typename FieldData::value_type;
  };

public:
  template <typename FieldData, typename Value, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field, active_index const &,
                  Value const &value, Data const &data) const {
    return field.value.set(data.active, value);
  }
};

struct EvaluateFieldTransform : boost::proto::callable {
public:
  template <typename Func> struct result;

public:
  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> &, Index &, Data &,
                     Args... args)> {
    using type = typename FieldData::value_type;
  };

  template <typename This, typename FieldData, typename Index, typename Data,
            typename... Args>
  struct result<This(varying_scalar<FieldData> const &, Index &, Data &,
                     Args... args)> {
    using type = typename FieldData::value_type;
  };

public:
  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field, active_index const &,
                  Data const &data, Args &&... args) const {
    return field.value.get(data.active, std::forward<Args>(args)...);
  }

  template <typename FieldData, typename Data, typename... Args>
  auto operator()(varying_scalar<FieldData> const &field,
                  neighbour_index const &, Data const &data,
                  Args &&... args) const {
    return field.value.get(data.active, std::forward<Args>(args)...);
  }
};

struct AssignTransform
    : boost::proto::when<
          boost::proto::assign<
              boost::proto::subscript<AnyFieldTerm, AnyIndexTerm>,
              boost::proto::_>,
          EvaluateAssignTransform(
              boost::proto::_value(
                  boost::proto::_child0(boost::proto::_child0)),
              boost::proto::_value(
                  boost::proto::_child1(boost::proto::_child0)),
              boost::proto::call<TransformSubscript<EvaluateFieldTransform>(
                  boost::proto::_child1, boost::proto::_data)>,
              boost::proto::_data)> {};

// struct AssignTransform
//    : boost::proto::when<
//          boost::proto::assign<boost::proto::_, boost::proto::_>,
//          boost::proto::_make_assign(
//              boost::proto::_child0,
//              boost::proto::_make_terminal(
//                  boost::proto::call<TransformSubscript<EvaluateFieldTransform>(
//                      boost::proto::_child1, boost::proto::_data)>))> {};

} // namespace detail

template <typename E>
auto evaluate_assign(E const &e, size_t active, size_t neighbour) {
  struct {
    size_t active;
    size_t neighbour;
  } data{active, neighbour};

  detail::AssignTransform transform;
  return transform(e, 0, data);
}

} // namespace prtcl::expr

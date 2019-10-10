#pragma once

#include "../../data/group_data.hpp"
#include "../../data/host/host_linear_access.hpp"
#include "../../meta/is_any_of.hpp"
#include "../../meta/remove_cvref.hpp"
#include "../../tags.hpp"
#include "../field_data.hpp"

#include <tuple>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

struct rw_access_field : boost::proto::callable {
  template <typename T> using rcvr_t = remove_cvref_t<T>;
  template <typename T> using kind_t = typename rcvr_t<T>::kind_tag;
  template <typename T> using type_t = typename rcvr_t<T>::type_tag;
  template <typename T> using group_t = typename rcvr_t<T>::group_tag;

  struct call_get_rw_access {
    template <typename Buffer, typename ArgsTuple>
    auto operator()(Buffer const &buffer, ArgsTuple &&args_tuple) {
      return std::apply(
          [&buffer](auto... args) {
            return get_rw_access(buffer, std::forward<decltype(args)>(args)...);
          },
          std::forward<ArgsTuple>(args_tuple));
    }
  };

  template <typename> struct result;

  template <typename This, typename FD, typename D> struct result<This(FD, D)> {
    using type =
        field_data<kind_t<FD>, type_t<FD>, group_t<FD>,
                   std::invoke_result_t<call_get_rw_access,
                                        decltype(std::declval<FD>().data), D>>;
  };

  template <typename FD, typename D>
  typename result<rw_access_field(FD, D)>::type operator()(FD const &fd,
                                                           D const &data) {
    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>)
      return {fd.index, std::invoke(call_get_rw_access{}, fd.data, data)};
    else
      return {std::invoke(call_get_rw_access{}, fd.data, data)};
  }
};

struct ro_access_field : boost::proto::callable {
  template <typename T> using rcvr_t = remove_cvref_t<T>;
  template <typename T> using kind_t = typename rcvr_t<T>::kind_tag;
  template <typename T> using type_t = typename rcvr_t<T>::type_tag;
  template <typename T> using group_t = typename rcvr_t<T>::group_tag;

  struct call_get_ro_access {
    template <typename Buffer, typename ArgsTuple>
    auto operator()(Buffer const &buffer, ArgsTuple &&args_tuple) {
      return std::apply(
          [&buffer](auto... args) {
            return get_ro_access(buffer, std::forward<decltype(args)>(args)...);
          },
          std::forward<ArgsTuple>(args_tuple));
    }
  };

  template <typename> struct result;

  template <typename This, typename FD, typename D> struct result<This(FD, D)> {
    using type =
        field_data<kind_t<FD>, type_t<FD>, group_t<FD>,
                   std::invoke_result_t<call_get_ro_access,
                                        decltype(std::declval<FD>().data), D>>;
  };

  template <typename FD, typename D>
  typename result<ro_access_field(FD, D)>::type operator()(FD const &fd,
                                                           D const &data) {
    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>)
      return {fd.index, std::invoke(call_get_ro_access{}, fd.data, data)};
    else
      return {std::invoke(call_get_ro_access{}, fd.data, data)};
  }
};

struct access_all_fields
    : boost::proto::or_<
          // RO-Access any top-level FieldData on the left hand side of an
          // assignment.
          boost::proto::when<boost::proto::assign<FieldData, boost::proto::_>,
                             boost::proto::_make_assign(
                                 rw_access_field(boost::proto::_value(
                                                     boost::proto::_child0),
                                                 boost::proto::_data),
                                 access_all_fields(boost::proto::_child1,
                                                   boost::proto::_data))>,
          // RW-Access any remaining field data.
          boost::proto::when<FieldData, ro_access_field(boost::proto::_value,
                                                        boost::proto::_data)>,
          // Terminals are stored by value and are not modified in this step.
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_byval(boost::proto::_)>,
          // Recurse into any Non-Terminals.
          boost::proto::nary_expr<boost::proto::_,
                                  boost::proto::vararg<access_all_fields>>> {};

} // namespace expression
} // namespace prtcl

#pragma once

#include "../../data/group_data.hpp"
#include "../../data/host/host_linear_access.hpp"
#include "../../meta/is_any_of.hpp"
#include "../../meta/remove_cvref.hpp"
#include "../../tags.hpp"
#include "../field_data.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

struct resolve_field : boost::proto::callable {
  template <typename> struct result;

  template <typename T> using rcvr_t = remove_cvref_t<T>;
  template <typename T> using kind_t = typename rcvr_t<T>::kind_tag;
  template <typename T> using type_t = typename rcvr_t<T>::type_tag;
  template <typename T> using group_t = typename rcvr_t<T>::group_tag;

  template <typename This, typename FD, typename GB>
  struct result<This(FD, GB)> {
    using scalars_type = typename rcvr_t<GB>::scalars_type;
    using vectors_type = typename rcvr_t<GB>::vectors_type;

    static_assert(is_any_of_v<kind_t<FD>, tag::uniform, tag::varying>);
    static_assert(is_any_of_v<type_t<FD>, tag::scalar, tag::vector>);

    using data_type =
        std::conditional_t<std::is_same<type_t<FD>, tag::scalar>::value,
                           scalars_type, vectors_type>;

    using type = field_data<kind_t<FD>, type_t<FD>, group_t<FD>, data_type>;
  };

  template <typename FD, typename GB>
  typename result<resolve_field(FD, GB)>::type operator()(FD const &fd,
                                                          GB const &group) {
    if constexpr (is_any_of_v<kind_t<FD>, tag::uniform>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::scalar>) {
        // std::cout << "NAME-INDEX: " << fd.data << " " <<
        // *group.get_uniform_scalar_index(fd.data) << std::endl;
        return {*group.get_uniform_scalar_index(fd.data),
                group.get_uniform_scalars()};
      }
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        return {*group.get_uniform_vector_index(fd.data),
                group.get_uniform_vectors()};
      throw "unknown field type";
    }
    if constexpr (is_any_of_v<kind_t<FD>, tag::varying>) {
      if constexpr (is_any_of_v<type_t<FD>, tag::scalar>)
        return {*group.get_varying_scalar(fd.data)};
      if constexpr (is_any_of_v<type_t<FD>, tag::vector>)
        return {*group.get_varying_vector(fd.data)};
      throw "unknown field type";
    }
    throw "unknown field kind";
  }
};

struct select_group : boost::proto::transform<select_group> {
  template <typename T> using group_t = typename remove_cvref_t<T>::group_tag;

  template <typename E, typename S, typename D>
  struct impl : boost::proto::transform_impl<E, S, D> {
    using field_type = typename boost::proto::result_of::value<E>::type;

    using group_type = remove_cvref_t<decltype(std::declval<D>().active)>;
    static_assert(
        std::is_same<group_type, remove_cvref_t<decltype(
                                     std::declval<D>().passive)>>::value,
        "active and passive groups must be of the same type");

    using result_type = group_type const &;

    result_type operator()(typename impl::expr_param,
                           typename impl::state_param,
                           typename impl::data_param d) const {
      static_assert(
          is_any_of_v<group_t<field_type>, tag::active, tag::passive>);
      return is_any_of_v<group_t<field_type>, tag::active> ? d.active
                                                           : d.passive;
    }
  };
};

struct resolve_all_fields
    : boost::proto::or_<
          // Subscripts of a FieldName by a Group are collapsed into a
          // field_data object.
          boost::proto::when<FieldData,
                             resolve_field(boost::proto::_value,
                                           select_group(boost::proto::_,
                                                        boost::proto::_state,
                                                        boost::proto::_data))>,
          // Terminals are stored by value and are not modified in this step.
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_byval(boost::proto::_)>,
          // Recurse into any Non-Terminals.
          boost::proto::nary_expr<boost::proto::_,
                                  boost::proto::vararg<resolve_all_fields>>> {};

} // namespace expression
} // namespace prtcl

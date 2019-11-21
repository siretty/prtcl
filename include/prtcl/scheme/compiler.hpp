#pragma once

#include "prtcl/meta/is_any_of.hpp"
#include "prtcl/tags.hpp"
#include <boost/hana/fwd/transform.hpp>
#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/expression.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_subscript_transform.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/scheme/compiler/block.hpp>
#include <prtcl/scheme/compiler/eq.hpp>
#include <prtcl/scheme/compiler/loop.hpp>

#include <prtcl/meta/format_cxx_type.hpp>

#include <type_traits>
#include <utility>
#include <vector>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::scheme {

template <typename Scalar, size_t N> class compiler {
public:
  using scheme_data = data::scheme<Scalar, N>;

private:
  struct name_transform {
  public:
    template <typename TT, typename GT, typename V>
    prtcl::expr::field_term<tag::global, TT, GT, size_t>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<tag::global, TT, GT, V> field_) const {
      if (auto index = data.get(tag::global{}, TT{}).get_index(field_.value))
        return {{*index}};
      else
        throw "unknown field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename V>
    prtcl::expr::field_term<KT, TT, tag::active, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<KT, TT, tag::active, V> field_) const {
      auto &group = data.get_group(gi_a);
      if (auto index = group.get(KT{}, TT{}).get_index(field_.value))
        return {{{gi_a, *index}}};
      else
        throw "unknown active field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename V>
    prtcl::expr::field_term<KT, TT, tag::passive, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               prtcl::expr::field<KT, TT, tag::passive, V> field_) const {
      auto &group = data.get_group(gi_p);
      if (auto index = group.get(KT{}, TT{}).get_index(field_.value))
        return {{{gi_p, *index}}};
      else
        throw "unknown passive field name (" + field_.value + ")";
    }

    /// Keep terminals by-value.
    template <typename Value>
    boost::yap::terminal<boost::yap::expression, meta::remove_cvref_t<Value>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               Value &&value) const {
      return {{std::forward<Value>(value)}};
    }

    scheme_data &data;
    size_t gi_a, gi_p;
  };

  struct eq_transform {
  public:
    template <typename KT, typename TT, typename GT, typename V, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                    prtcl::expr::field<KT, TT, GT, V> lhs, RHS &&rhs) const {
      return eq<eq_kind::normal, eq_operator::none, decltype(lhs),
                meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
    }

    template <typename KT, typename TT, typename GT, typename V, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                    prtcl::expr::field<KT, TT, GT, V> lhs, RHS &&rhs) const {
      if constexpr (meta::is_any_of_v<KT, tag::varying>)
        return eq<eq_kind::normal, eq_operator::add, decltype(lhs),
                  meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
      else
        return eq<eq_kind::reduce, eq_operator::add, decltype(lhs),
                  meta::remove_cvref_t<RHS>>{lhs, std::forward<RHS>(rhs)};
    }
  };

  struct split_transform {
    struct inner {
      template <typename Arg> auto _transform(size_t gi_p, Arg &&e0) const {
        auto e1 = boost::yap::transform(std::forward<Arg>(e0),
                                        name_transform{data, gi_a, gi_p});
        auto e2 = boost::yap::transform(e1, eq_transform{});
        return e2;
      }

      template <typename... Args>
      auto _collect(size_t gi_p, Args &&... args_) const {
        return boost::hana::make_tuple(
            _transform(gi_p, boost::yap::as_expr(args_))...);
      }

      template <typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      prtcl::expr::loop<Select> loop_, Args &&... args_) const {
        using tuple = decltype(_collect(0, std::forward<Args>(args_)...));
        loop<block<tuple>> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          auto &group = data.get_group(gi_p);
          if (loop_.select(group)) {
            result.groups.push_back(gi_p);
            result.instances.push_back(
                block<tuple>{_collect(gi_p, std::forward<Args>(args_)...)});
          }
        }
        return std::move(result);
      }

      template <boost::yap::expr_kind Kind, typename... Args>
      auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
        return _transform(gi_a, boost::yap::make_expression<Kind>(
                                    std::forward<Args>(args_)...));
      }

      scheme_data &data;
      size_t gi_a;
    };

    template <typename... Args>
    auto _call_inner(size_t gi_a, Args &&... args_) const {
      return boost::hana::make_tuple(boost::yap::transform(
          boost::yap::as_expr(args_), inner{data, gi_a})...);
    }

    template <typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    prtcl::expr::loop<Select> loop_, Args &&... args_) const {
      using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      loop<block<tuple>> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        auto &group = data.get_group(gi_a);
        if (loop_.select(group)) {
          result.groups.push_back(gi_a);
          result.instances.push_back(
              block<tuple>{_call_inner(gi_a, std::forward<Args>(args_)...)});
        }
      }
      return std::move(result);
    }

    scheme_data &data;
  };

public:
  template <typename E0> auto operator()(E0 &&e0) {
    auto e1 =
        boost::yap::transform(e0, prtcl::expr::field_subscript_transform{});

    display_cxx_type(e1, std::cout);
    boost::yap::print(std::cout, e1);

    auto e2 = boost::yap::transform(e1, split_transform{_data});

    display_cxx_type(e2, std::cout);
    // boost::yap::print(std::cout, e2);

    return std::move(e2);
  }

public:
  compiler(scheme_data &data_) : _data{data_} {}

private:
  scheme_data &_data;
};

} // namespace prtcl::scheme

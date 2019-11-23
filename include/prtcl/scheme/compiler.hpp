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
#include <prtcl/expr/reduction.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/scheme/compiler/block.hpp>
#include <prtcl/scheme/compiler/eq.hpp>
#include <prtcl/scheme/compiler/loop.hpp>
#include <prtcl/scheme/compiler/stmt.hpp>
#include <prtcl/scheme/compiler/unbound_loop.hpp>

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

  struct stmt_transform {
  private:
    static constexpr auto allowed_kinds = boost::hana::make_tuple(
        boost::yap::expr_kind::assign, boost::yap::expr_kind::plus_assign,
        boost::yap::expr_kind::minus_assign);

  public:
    template <boost::yap::expr_kind Kind, typename TT, typename GT, typename V,
              typename RHS,
              typename = std::enable_if_t<boost::hana::in(Kind, allowed_kinds)>>
    auto operator()(boost::yap::expr_tag<Kind>,
                    prtcl::expr::field<tag::varying, TT, GT, V> lhs,
                    RHS &&rhs) const {
      using expr_type = decltype(
          boost::yap::make_expression<Kind>(lhs, std::forward<RHS>(rhs)));
      return stmt<expr_type>{
          boost::yap::make_expression<Kind>(lhs, std::forward<RHS>(rhs))};
    }
  };

  struct eq_transform {
  public:
    template <prtcl::expr::reduction_op Op, typename LHS, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    prtcl::expr::reduction<Op>, LHS &&lhs, RHS &&rhs) const {
      return eq<eq_kind::reduce, eq_operator::max, meta::remove_cvref_t<LHS>,
                meta::remove_cvref_t<RHS>>{std::forward<LHS>(lhs),
                                           std::forward<RHS>(rhs)};
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
        return make_block(_transform(gi_p, boost::yap::as_expr(args_))...);
      }

      template <typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      prtcl::expr::loop<Select> loop_, Args &&... args_) const {
        using block_type = decltype(_collect(0, std::forward<Args>(args_)...));
        loop<block_type> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          auto &group = data.get_group(gi_p);
          if (loop_.select(group)) {
            result.groups.push_back(gi_p);
            result.instances.push_back(
                _collect(0, std::forward<Args>(args_)...));
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
      return make_block(boost::yap::transform(boost::yap::as_expr(args_),
                                              inner{data, gi_a})...);
    }

    template <typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    prtcl::expr::loop<Select> loop_, Args &&... args_) const {
      using block_type = decltype(_call_inner(0, std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      loop<block_type> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        auto &group = data.get_group(gi_a);
        if (loop_.select(group)) {
          result.groups.push_back(gi_a);
          result.instances.push_back(
              _call_inner(0, std::forward<Args>(args_)...));
        }
      }
      return std::move(result);
    }

    scheme_data &data;
  };

  struct block_transform {
    struct inner_block_transform {
      template <typename Arg> auto _transform(Arg &&arg_) const {
        return boost::yap::transform(std::forward<Arg>(arg_), stmt_transform{},
                                     eq_transform{});
      }

      template <typename... Args> auto _collect(Args &&... args_) const {
        return make_block(_transform(boost::yap::as_expr(args_))...);
      }

      template <typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      prtcl::expr::loop<Select> loop_, Args &&... args_) const {
        using block_type = decltype(_collect(std::forward<Args>(args_)...));
        return unbound_loop<Select, block_type>{
            loop_.select, _collect(std::forward<Args>(args_)...)};
      }

      template <boost::yap::expr_kind Kind, typename... Args>
      auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
        return _transform(
            boost::yap::make_expression<Kind>(std::forward<Args>(args_)...));
      }
    };

    template <typename... Args> auto _collect(Args &&... args_) const {
      return make_block(boost::yap::transform(boost::yap::as_expr(args_),
                                              inner_block_transform{})...);
    }

    template <typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    prtcl::expr::loop<Select> loop_, Args &&... args_) const {
      using block_type = decltype(_collect(std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      return unbound_loop<Select, block_type>{
          loop_.select, _collect(std::forward<Args>(args_)...)};
    }
  };

public:
  template <typename E0> auto operator()(E0 &&e0) {
    auto e1 =
        boost::yap::transform(e0, prtcl::expr::field_subscript_transform{});

    display_cxx_type(e1, std::cout);
    boost::yap::print(std::cout, e1);

    auto e2 = boost::yap::transform(e1, block_transform{});

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

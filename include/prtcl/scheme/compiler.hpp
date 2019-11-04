#pragma once

#include <boost/hana/fwd/transform.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_access_transform.hpp>
#include <prtcl/expr/field_subscript_transform.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/remove_cvref.hpp>

#include <prtcl/meta/format_cxx_type.hpp>

#include <utility>
#include <vector>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::scheme {

template <typename Tuple> struct loop {
  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using tuple = decltype(boost::hana::transform(std::declval<Tuple>(),
                                                  std::declval<Transform>()));
    // build the resulting loop
    loop<tuple> result{groups, {}};
    for (auto &e : expressions) {
      result.expressions.emplace_back(
          boost::hana::transform(e, std::forward<Transform>(transform)));
    }
    return std::move(result);
  }

  std::vector<size_t> groups;
  std::vector<Tuple> expressions;
};

enum class eq_kind { assign, accumulate, reduce };
enum class eq_operator { none, add, sub, mul, div, max, min };

template <eq_kind Kind, eq_operator Op, typename LHS, typename RHS> struct eq {
  static_assert(Kind == eq_kind::assign ? Op == eq_operator::none
                                        : Op != eq_operator::none);

  template <typename Transform> auto transform(Transform &&transform) const {
    // find the type of the tuple of transformed expressions
    using lhs_type = decltype(std::declval<Transform>()(std::declval<LHS>()));
    using rhs_type = decltype(std::declval<Transform>()(std::declval<RHS>()));
    // return the resulting reduction
    return eq<Kind, Op, lhs_type, rhs_type>{
        std::forward<Transform>(transform)(lhs),
        std::forward<Transform>(transform)(rhs)};
  }

  LHS lhs;
  RHS rhs;
};

template <typename> struct is_loop : std::false_type {};
template <typename Tuple> struct is_loop<loop<Tuple>> : std::true_type {};

template <typename T>
constexpr bool is_loop_v = is_loop<meta::remove_cvref_t<T>>::value;

template <typename Scalar, size_t N> class compiler {
public:
  using scheme_data = data::scheme<Scalar, N>;

private:
  // generic, not openmp-specific
  struct name_transform {
  public:
    template <typename TT, typename GT, typename AT, typename V>
    expr::field_term<tag::global, TT, GT, AT, size_t>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::global, TT, GT, AT, V> field_) const {
      if (auto index = data.get(tag::global{}, TT{}).get_index(field_.value))
        return {{*index}};
      else
        throw "unknown field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename AT, typename V>
    expr::field_term<KT, TT, tag::active, AT, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<KT, TT, tag::active, AT, V> field_) const {
      auto &group = data.get_group(gi_a);
      if (auto index = group.get(KT{}, TT{}).get_index(field_.value))
        return {{{gi_a, *index}}};
      else
        throw "unknown active field name (" + field_.value + ")";
    }

    template <typename KT, typename TT, typename AT, typename V>
    expr::field_term<KT, TT, tag::passive, AT, std::pair<size_t, size_t>>
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<KT, TT, tag::passive, AT, V> field_) const {
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

  // generic, not openmp-specific
  struct split_transform {
    struct inner {
      template <typename Arg> auto _transform(size_t gi_p, Arg &&arg) const {
        return boost::yap::transform(std::forward<Arg>(arg),
                                     name_transform{data, gi_a, gi_p});
      }

      template <typename... Args>
      auto _collect(size_t gi_p, Args &&... args_) const {
        return boost::hana::make_tuple(
            _transform(gi_p, boost::yap::as_expr(args_))...);
      }

      template <typename GT, typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      expr::loop<GT, Select> loop_, Args &&... args_) const {
        using tuple = decltype(_collect(0, std::forward<Args>(args_)...));
        loop<tuple> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          auto &group = data.get_group(gi_p);
          if (loop_.select(group)) {
            result.groups.push_back(gi_p);
            result.expressions.push_back(
                _collect(gi_p, std::forward<Args>(args_)...));
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

    template <typename GT, typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::loop<GT, Select> loop_, Args &&... args_) const {
      using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      loop<tuple> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        auto &group = data.get_group(gi_a);
        if (loop_.select(group)) {
          result.groups.push_back(gi_a);
          result.expressions.push_back(
              _call_inner(gi_a, std::forward<Args>(args_)...));
        }
      }
      return std::move(result);
    }

    scheme_data &data;
  };

public:
  template <typename E0> auto operator()(E0 &&e0) {
    auto e1 = boost::yap::transform(e0, expr::field_subscript_transform{});

    display_cxx_type(e1, std::cout);
    boost::yap::print(std::cout, e1);

    auto e2 = boost::yap::transform(e1, expr::field_access_transform{});
    auto e3 = boost::yap::transform(e2, split_transform{_data});

    display_cxx_type(e3, std::cout);
    // boost::yap::print(std::cout, e3);

    return std::move(e3);
  }

public:
  compiler(scheme_data &data_) : _data{data_} {}

private:
  scheme_data &_data;
};

} // namespace prtcl::scheme

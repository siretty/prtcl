#pragma once

#include "prtcl/meta/remove_cvref.hpp"
#include "prtcl/tags.hpp"
#include <boost/yap/algorithm_fwd.hpp>
#include <prtcl/data/openmp/scheme.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/call.hpp>
#include <prtcl/expr/field_access_transform.hpp>
#include <prtcl/expr/field_subscript_transform.hpp>
#include <prtcl/expr/field_value_transform.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/format_cxx_type.hpp>
#include <prtcl/meta/overload.hpp>

#include <iostream>

#include <cstddef>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::scheme::openmp {

struct default_functions {
  auto get_function(tag::dot) const {
    return [](auto &&lhs, auto &&rhs) {
      return std::forward<decltype(lhs)>(lhs).matrix().dot(
          std::forward<decltype(rhs)>(rhs).matrix());
    };
  }

  auto get_function(tag::norm) const {
    return [](auto &&arg) {
      return std::forward<decltype(arg)>(arg).matrix().norm();
    };
  }

  auto get_function(tag::norm_squared) const {
    return [](auto &&arg) {
      return std::forward<decltype(arg)>(arg).matrix().squaredNorm();
    };
  }

  auto get_function(tag::normalized) const {
    return [](auto &&arg) {
      return std::forward<decltype(arg)>(arg).normalized();
    };
  }

  auto get_function(tag::min) const {
    return [](auto &&... args) {
      return std::min(std::forward<decltype(args)>(args)...);
    };
  }

  auto get_function(tag::max) const {
    return [](auto &&... args) {
      return std::max(std::forward<decltype(args)>(args)...);
    };
  }

  // auto get_function(tag::kernel) const {
  //  return [k = _kernel, h = _smoothing_scale](auto delta_x) {
  //    return k.eval(delta_x, h);
  //  };
  //}

  // auto get_function(tag::kernel_gradient) const {
  //  return [k = _kernel, h = _smoothing_scale](auto delta_x) {
  //    return k.evalgrad(delta_x, h);
  //  };
  //}
};

template <typename Tuple> struct active_loop {
  struct eval_transform {
  public:
    template <typename TT, typename AT, typename V>
    auto operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        expr::field<tag::varying, TT, tag::active, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second][ri_a]);
    }

    template <typename TT, typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::global, TT, GT, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second]);
    }

    template <typename TT, typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, TT, GT, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second]);
    }

    size_t ri_a;
  };

  void operator()() const {
    std::cout << "active_loop\n";
    for (size_t n = 0; n < counts.size(); ++n) {
      std::cout << "group " << groups[n] << "\n";
      for (size_t ri_a = 0; ri_a < counts[n]; ++ri_a) {
        boost::hana::for_each(expressions[n], [ri_a](auto &&arg) {
          if constexpr (boost::yap::is_expr<
                            meta::remove_cvref_t<decltype(arg)>>::value) {
            auto e = boost::yap::transform(std::forward<decltype(arg)>(arg),
                                           eval_transform{ri_a});
            // display_cxx_type(e, std::cout);
            boost::yap::print(std::cout, e);
            boost::yap::evaluate(e);
          } else {
            std::forward<decltype(arg)>(arg)(ri_a);
          }
        });
      }
    }
  }

  std::vector<size_t> groups;
  std::vector<size_t> counts;
  std::vector<Tuple> expressions;
};

template <typename Tuple> struct passive_loop {
  struct eval_transform {
    template <typename TT, typename AT, typename V>
    auto operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        expr::field<tag::varying, TT, tag::active, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second][ri_a]);
    }

    template <typename TT, typename AT, typename V>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                    expr::field<tag::varying, TT, tag::passive, AT, V> const
                        &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second][ri_p]);
    }

    template <typename TT, typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::global, TT, GT, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second]);
    }

    template <typename TT, typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, TT, GT, AT, V> const &field_) const {
      return boost::yap::as_expr(field_.value.first[field_.value.second]);
    }

    size_t ri_a, ri_p;
  };

  void operator()(size_t ri_a) const {
    std::cout << "passive_loop\n";
    for (size_t n = 0; n < counts.size(); ++n) {
      std::cout << "group " << groups[n] << "\n";
      for (size_t ri_p = 0; ri_p < counts[n]; ++ri_p) {
        boost::hana::for_each(expressions[n], [ri_a, ri_p](auto &&arg) {
          auto e = boost::yap::transform(std::forward<decltype(arg)>(arg),
                                         eval_transform{ri_a, ri_p});
          // display_cxx_type(e, std::cout);
          boost::yap::print(std::cout, e);
          boost::yap::evaluate(e);
        });
      }
    }
  }

  std::vector<size_t> groups;
  std::vector<size_t> counts;
  std::vector<Tuple> expressions;
};

template <typename Scalar, size_t N> class compiler {
public:
  using scheme_data = data::scheme<Scalar, N>;
  using openmp_scheme_data = data::openmp::scheme<Scalar, N>;

private:
  // openmp-specific
  struct data_transform {
  public:
    template <typename TT, typename GT, typename AT>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                    expr::field<tag::global, TT, GT, AT, size_t> field_) const {
      auto &field_data = openmp_data.get(tag::global{}, TT{});
      using value_type =
          std::pair<meta::remove_cvref_t<decltype(field_data)>, size_t>;
      return expr::field_term<tag::global, TT, GT, AT, value_type>{
          {{field_data, field_.value}}};
    }

    template <typename KT, typename TT, typename GT, typename AT>
    auto operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        expr::field<KT, TT, GT, AT, std::pair<size_t, size_t>> field_) const {
      auto &field_data =
          openmp_data.get_group(field_.value.first).get(KT{}, TT{});
      using value_type =
          std::pair<meta::remove_cvref_t<decltype(field_data)>, size_t>;
      return expr::field_term<KT, TT, GT, AT, value_type>{
          {{field_data, field_.value.second}}};
    }

    openmp_scheme_data &openmp_data;
  };

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
        return boost::yap::transform(
            boost::yap::transform(std::forward<Arg>(arg),
                                  name_transform{data, gi_a, gi_p}),
            data_transform{openmp_data});
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
        passive_loop<tuple> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          auto &group = data.get_group(gi_p);
          if (loop_.select(group)) {
            result.groups.push_back(gi_p);
            result.counts.push_back(group.size());
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

      openmp_scheme_data &openmp_data;
    };

    template <typename... Args>
    auto _call_inner(size_t gi_a, Args &&... args_) const {
      return boost::hana::make_tuple(boost::yap::transform(
          boost::yap::as_expr(args_), inner{data, gi_a, openmp_data})...);
    }

    template <typename GT, typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::loop<GT, Select> loop_, Args &&... args_) const {
      using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
      // result stores transformed sub-expressions of all selected groups
      active_loop<tuple> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        auto &group = data.get_group(gi_a);
        if (loop_.select(group)) {
          result.groups.push_back(gi_a);
          result.counts.push_back(group.size());
          result.expressions.push_back(
              _call_inner(gi_a, std::forward<Args>(args_)...));
        }
      }
      return std::move(result);
    }

    scheme_data &data;

    openmp_scheme_data &openmp_data;
  };

  template <typename Functions> struct call_transform {
    template <typename FT, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::call<FT>, Args &&... args) const {
      namespace yap = boost::yap;
      return yap::make_expression<yap::expr_kind::call>(
          yap::as_expr(functions.get_function(FT{})),
          yap::as_expr(std::forward<Args>(args))...);
    }

    Functions functions;
  };

public:
  template <typename E0> auto operator()(E0 &&e0) {
    // generic, not openmp-specific
    auto e1 = boost::yap::transform(e0, expr::field_subscript_transform{});
    auto e2 = boost::yap::transform(e1, expr::field_access_transform{});

    // openmp-specific
    auto e3 = boost::yap::transform(e2, call_transform<default_functions>{{}});

    // partially not openmp-specific (generic: name_transform)
    auto e4 = boost::yap::transform(e3, split_transform{_data, _openmp_data});

    // display_cxx_type(e4, std::cout);
    // boost::yap::print(std::cout, e4);

    return std::move(e4);
  }

public:
  compiler(scheme_data &data_, openmp_scheme_data &openmp_data_)
      : _data{data_}, _openmp_data{openmp_data_} {}

private:
  scheme_data &_data;
  openmp_scheme_data &_openmp_data;
};

} // namespace prtcl::scheme::openmp

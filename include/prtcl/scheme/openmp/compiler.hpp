#pragma once

#include <prtcl/data/scheme.hpp>
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

template <typename Scalar, size_t N> class compiler {
public:
  using scheme_data = data::scheme<Scalar, N>;

private:
  struct data_transform {
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

    scheme_data &data;
    size_t gi_a, gi_p;
  };

  struct split_transform {
    struct inner {
      template <typename... Args>
      auto _collect(size_t gi_p, Args &&... args_) const {
        return boost::hana::make_tuple(boost::yap::transform(
            boost::yap::as_expr(args_), data_transform{data, gi_a, gi_p})...);
      }

      template <typename GT, typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      expr::loop<GT, Select> loop_, Args &&... args_) const {
        using tuple = decltype(_collect(0, std::forward<Args>(args_)...));
        std::vector<std::optional<tuple>> result;
        for (size_t gi_p = 0; gi_p < data.get_group_count(); ++gi_p) {
          // store only transformed sub-expressions if the group was selected
          if (loop_.select(data.get_group(gi_p))) {
            result.push_back(_collect(gi_p, std::forward<Args>(args_)...));
          }
        }
        return std::move(result);
      }

      template <boost::yap::expr_kind Kind, typename... Args>
      auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
        return boost::yap::transform(
            boost::yap::make_expression<Kind>(std::forward<Args>(args_)...),
            data_transform{data, gi_a, gi_a});
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
      std::vector<std::optional<tuple>> result;
      // iterate over all (active) groups
      for (size_t gi_a = 0; gi_a < data.get_group_count(); ++gi_a) {
        // store only transformed sub-expressions if the group was selected
        if (loop_.select(data.get_group(gi_a))) {
          result.push_back(_call_inner(gi_a, std::forward<Args>(args_)...));
        }
      }
      return std::move(result);
    }

    scheme_data &data;
  };

public:
  compiler(scheme_data &data_) : _data{data_} {}

public:
  template <typename E0> auto operator()(E0 &&e0) {
    auto e1 = boost::yap::transform(e0, expr::field_subscript_transform{});
    auto e2 = boost::yap::transform(e1, expr::field_access_transform{});

    boost::yap::print(std::cout, e2);

    auto e3 = boost::yap::transform(e2, split_transform{_data});

    display_cxx_type(e3, std::cout);
    // boost::yap::print(std::cout, e3);
  }

private:
  scheme_data &_data;
};

} // namespace prtcl::scheme::openmp

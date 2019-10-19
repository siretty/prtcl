#pragma once

// ============================================================

#include "../data/group_data.hpp"
#include "../math/kernel/cubic_spline_kernel.hpp"

namespace prtcl {

template <typename Vector, typename VectorAccess> struct host_scheme_group {
  using vector_type = Vector;
  VectorAccess position;

  friend size_t get_element_count(host_scheme_group const &g) {
    return g.position.size();
  }

  friend auto get_element_ref(host_scheme_group const &g, size_t index) {
    return g.position.template get<Vector>(index);
  }
};

template <typename ExecutionTag, typename MathTraits> class scheme_data {
public:
  using math_traits = MathTraits;
  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;
  constexpr static size_t vector_extent = math_traits::vector_extent;

public:
  using group_data_type = group_data<scalar_type, vector_extent>;
  using group_buffer_type =
      typename result_of::get_buffer<group_data_type, ExecutionTag>::type;

private:
  std::vector<group_data_type> group_data_;
  std::vector<group_buffer_type> group_buffer_;

public:
  size_t add_group() {
    size_t index = group_data_.size();
    group_data_.push_back({});
    return index;
  }

  group_data_type &get_group(size_t index) {
    if (index >= group_data_.size())
      throw "index out of bounds";
    return group_data_[index];
  }

  group_buffer_type const &get_group_buffer(size_t index) const {
    if (index >= group_buffer_.size())
      throw "index out of bounds";
    return group_buffer_[index];
  }

  size_t get_group_count() const { return group_data_.size(); }

  friend size_t get_group_count(scheme_data const &scheme) {
    return scheme.get_group_count();
  }

  friend auto get_group_ref(scheme_data const &scheme, size_t index) {
    auto x_access = get_ro_access(
        *scheme.group_buffer_[index].get_varying_vector("position"));
    return host_scheme_group<vector_type, decltype(x_access)>{x_access};
  }

public:
  void create_buffers() {
    group_buffer_.clear();

    // create a buffer object for each data object
    for (auto &gd : group_data_)
      group_buffer_.emplace_back(get_buffer(gd, tag::host{}));
  }

  void destroy_buffers() { group_buffer_.clear(); }
};

} // namespace prtcl

// ============================================================

#include "../access/uniform.hpp"
#include "../access/varying.hpp"
#include "../data/host/grouped_uniform_grid.hpp"
#include "../expr/call.hpp"
#include "../expr/field.hpp"
#include "../expr/field_access_transform.hpp"
#include "../expr/field_subscript_transform.hpp"
#include "../expr/loop.hpp"

#include "../expr/text_transform.hpp"

#include "../meta/format_cxx_type.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

#include <cstddef>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

#include <omp.h>

namespace prtcl {

template <typename Impl, typename MathTraits,
          template <typename> typename KernelT = cubic_spline_kernel>
struct host_scheme : scheme_data<tag::host, MathTraits> {
private:
  using impl_type = Impl;

  using math_traits = MathTraits;

public:
  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;
  constexpr static size_t vector_extent = math_traits::vector_extent;

  using kernel_type = KernelT<math_traits>;

  //#define PRTCL_DEBUG

  // call_transform {{{

private:
  struct call_transform {
    template <typename FT, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::call<FT>, Args &&... args) const {
      namespace yap = boost::yap;
      return yap::make_expression<yap::expr_kind::call>(
          yap::as_expr(impl.get_function(FT{})),
          yap::as_expr(std::forward<Args>(args))...);
    }

    impl_type &impl;
  };

  // }}}

  // data_transform {{{

private:
  struct data_transform {
  private:
    auto const &get_group_buffer(tag::active) const {
      auto const &buffer = data.get_group_buffer(gi_a);
#ifdef PRTCL_DEBUG
      std::cout << "get_group_buffer(active) a=" << gi_a << " " << &buffer
                << "\n";
#endif
      return buffer;
    }

    auto const &get_group_buffer(tag::passive) const {
      auto const &buffer = data.get_group_buffer(gi_p);
#ifdef PRTCL_DEBUG
      std::cout << "get_group_buffer(passive) p=" << gi_p << " " << &buffer
                << "\n";
#endif
      return buffer;
    }

  private:
    template <typename Buffer, typename... Args>
    static auto get_access(tag::read, Buffer &&buffer, Args &&... args) {
      return get_ro_access(std::forward<Buffer>(buffer),
                           std::forward<Args>(args)...);
    }

    template <typename Buffer, typename... Args>
    static auto get_access(tag::read_write, Buffer &&buffer, Args &&... args) {
      return get_rw_access(std::forward<Buffer>(buffer),
                           std::forward<Args>(args)...);
    }

  public:
    template <typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, tag::scalar, GT, AT, V> field_) const {
      auto const &buffer = get_group_buffer(GT{});
      auto index = buffer.get_uniform_scalar_index(field_.value);
      if (!index)
        throw "uniform scalar not found " + field_.value;
      auto access = access::make_uniform<AT>(
          get_access(AT{}, buffer.get_uniform_scalars()), *index);
      return expr::field_term<tag::uniform, tag::scalar, GT, AT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
    }

    template <typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, tag::vector, GT, AT, V> field_) const {
      auto const &buffer = get_group_buffer(GT{});
      auto index = buffer.get_uniform_vector_index(field_.value);
      if (!index)
        throw "uniform vector not found " + field_.value;
      auto access = access::make_uniform<AT>(
          get_access(AT{}, buffer.get_uniform_vectors()), *index);
      return expr::field_term<tag::uniform, tag::vector, GT, AT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
      // return boost::yap::make_terminal(access::make_uniform<AT>(
      //    get_access(AT{}, buffer.get_uniform_vectors()),
      //    *buffer.get_uniform_vector_index(field_.value)));
    }

    template <typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::varying, tag::scalar, GT, AT, V> field_) const {
      auto const &buffer = get_group_buffer(GT{});
      auto value = buffer.get_varying_scalar(field_.value);
      if (!value)
        throw "varying scalar not found " + field_.value;
      auto access = access::make_varying<AT>(get_access(AT{}, *value));
      return expr::field_term<tag::varying, tag::scalar, GT, AT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
      // return boost::yap::make_terminal(access::make_varying<AT>(
      //    get_access(AT{}, *buffer.get_varying_scalar(field_.value))));
    }

    template <typename GT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::varying, tag::vector, GT, AT, V> field_) const {
      auto const &buffer = get_group_buffer(GT{});
      auto value = buffer.get_varying_vector(field_.value);
      if (!value)
        throw "varying vector not found " + field_.value;
      auto access = access::make_varying<AT>(get_access(AT{}, *value));
      return expr::field_term<tag::varying, tag::vector, GT, AT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
      // return boost::yap::make_terminal(access::make_varying<AT>(
      //    get_access(AT{}, *buffer.get_varying_vector(field_.value))));
    }

    host_scheme &data;
    size_t gi_a, gi_p;
  }; // namespace prtcl

  // }}}

  // split_transform {{{

private:
  struct split_transform {
    struct inner {
      template <typename... Args>
      auto _collect(size_t gi_p, Args &&... args_) const {
        return boost::hana::make_tuple(boost::yap::transform(
            boost::yap::as_expr(args_), data_transform{impl, gi_a, gi_p})...);
      }

      template <typename Select, typename... Args>
      auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                      expr::loop<tag::passive, Select> loop_,
                      Args &&... args_) const {
        using tuple = decltype(_collect(0, std::forward<Args>(args_)...));
        std::vector<std::optional<tuple>> result;
        for (size_t gi_p = 0; gi_p < impl.get_group_count(); ++gi_p) {
          if (loop_.select(impl.get_group(gi_p)))
            result.push_back(_collect(gi_p, std::forward<Args>(args_)...));
          else
            result.push_back(std::nullopt);
        }
        return std::move(result);
      }

      template <boost::yap::expr_kind Kind, typename... Args>
      auto operator()(boost::yap::expr_tag<Kind>, Args &&... args_) const {
        return boost::yap::transform(
            boost::yap::make_expression<Kind>(std::forward<Args>(args_)...),
            data_transform{impl, gi_a, gi_a});
      }

      impl_type &impl;
      size_t gi_a;
    };

    template <typename... Args>
    auto _call_inner(size_t gi_a, Args &&... args_) const {
      return boost::hana::make_tuple(boost::yap::transform(
          boost::yap::as_expr(args_), inner{impl, gi_a})...);
    }

    template <typename Select, typename... Args>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::loop<tag::active, Select> loop_,
                    Args &&... args_) const {
      using tuple = decltype(_call_inner(0, std::forward<Args>(args_)...));
      std::vector<std::optional<tuple>> result;
      for (size_t gi_a = 0; gi_a < impl.get_group_count(); ++gi_a) {
        if (loop_.select(impl.get_group(gi_a)))
          result.push_back(_call_inner(gi_a, std::forward<Args>(args_)...));
        else
          result.push_back(std::nullopt);
      }
      return std::move(result);
    }

    impl_type &impl;
  };

  // }}}

  // eval_transform {{{

private:
  struct eval_transform {
  private:
    template <typename Arg> auto _eval(tag::scalar, Arg &&arg) const {
      auto arg1 = boost::yap::transform(
          boost::yap::as_expr(std::forward<Arg>(arg)), *this);
#ifdef PRTCL_DEBUG
      boost::yap::print(std::cout, arg1);
      display_cxx_type(arg1, std::cout);
#endif
      // create a temporary so that no expression template (from Eigen) is
      // returned here, otherwise dangling references _will_ crash the program
      scalar_type result = boost::yap::evaluate(arg1);
      return result;
    }

    template <typename Arg> auto _eval(tag::vector, Arg &&arg) const {
      auto arg1 = boost::yap::transform(
          boost::yap::as_expr(std::forward<Arg>(arg)), *this);
#ifdef PRTCL_DEBUG
      boost::yap::print(std::cout, arg1);
      display_cxx_type(arg1, std::cout);
#endif
      // create a temporary so that no expression template (from Eigen) is
      // returned here, otherwise dangling references _will_ crash the program
      vector_type result = boost::yap::evaluate(arg1);
      return result;
    }

    template <typename Access, typename... Args>
    static auto get(tag::scalar, Access &&access, Args &&... args) {
      return std::forward<Access>(access).template get<scalar_type>(
          std::forward<Args>(args)...);
    }

    template <typename Access, typename... Args>
    static auto get(tag::vector, Access &&access, Args &&... args) {
      return std::forward<Access>(access).template get<vector_type>(
          std::forward<Args>(args)...);
    }

  public:
    template <typename KT, typename TT, typename AT, typename V, typename RHS>
    void operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                    expr::field<KT, TT, tag::active, AT, V> const &field_,
                    RHS &&rhs) const {
#ifdef PRTCL_DEBUG
      if constexpr (is_any_of_v<TT, tag::vector>) {
        vector_type rhs_value = _eval(TT{}, std::forward<RHS>(rhs));
        std::cout << "setting " << rhs_value << "\n";
      }
#endif
      field_.value.set(ri_a, _eval(TT{}, std::forward<RHS>(rhs)));
    }

    template <typename KT, typename TT, typename AT, typename V, typename RHS>
    void operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                    expr::field<KT, TT, tag::active, AT, V> const &field_,
                    RHS &&rhs) const {
      field_.value.set(ri_a, get(TT{}, field_.value, ri_a) +
                                 _eval(TT{}, std::forward<RHS>(rhs)));
    }

    template <typename KT, typename TT, typename AT, typename V, typename RHS>
    void operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus_assign>,
                    expr::field<KT, TT, tag::active, AT, V> const &field_,
                    RHS &&rhs) const {
      field_.value.set(ri_a, get(TT{}, field_.value, ri_a) -
                                 _eval(TT{}, std::forward<RHS>(rhs)));
    }

    template <typename KT, typename TT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<KT, TT, tag::active, AT, V> const &field_) const {
      return boost::yap::as_expr(get(TT{}, field_.value, ri_a));
    }

    template <typename KT, typename TT, typename AT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<KT, TT, tag::passive, AT, V> const &field_) const {
      return boost::yap::as_expr(get(TT{}, field_.value, ri_p));
    }

    size_t ri_a, ri_p;
  };

  // }}}

public:
  auto get_smoothing_scale() const { return _smoothing_scale; }

  void set_smoothing_scale(scalar_type value) {
    _smoothing_scale = value;
    auto grid_radius = _kernel.get_support_radius(_smoothing_scale);
    std::cout << "grid radius = " << grid_radius << "\n";
    _grid.set_radius(grid_radius);
  }

protected:
  void update_neighbourhoods() { _grid.update(*this); }

protected:
  template <typename Select, typename... Exprs>
  void for_each(Select &&select, Exprs &&... exprs) {
    // wrap the expressions into an active loop
    auto e0 = (expr::active_loop<remove_cvref_t<Select>>{
        {std::forward<Select>(select)}})(std::forward<Exprs>(exprs)...);

    // bind field subscripts
    auto e1 = boost::yap::transform(e0, expr::field_subscript_transform{});

    // mark field terminals with read / read_write access
    auto e2 = boost::yap::transform(e1, expr::field_access_transform{});

    // bind function calls
    auto e3 = boost::yap::transform(e2, call_transform{impl()});

    // boost::yap::print(std::cout, e3);

    auto bound_exprs = boost::yap::transform(e3, split_transform{impl()});
    // display_cxx_type(bound_exprs, std::cout);

    std::vector<std::vector<size_t>> neighbours;

    size_t total_paricles = 0;
    size_t total_neighbours = 0;

#pragma omp parallel private(neighbours)
    {
      neighbours.resize(this->get_group_count());

      for (size_t gi_a = 0; gi_a < this->get_group_count(); ++gi_a) {
        // skip group if not selected
        if (!bound_exprs[gi_a])
          continue;

#pragma omp for
        for (size_t ri_a = 0; ri_a < this->get_group(gi_a).size(); ++ri_a) {

#ifdef PRTCL_DEBUG
          std::cout << "a(" << gi_a << ", " << ri_a << ")\n";
#endif

          if (gi_a == 0) {
#pragma omp atomic
            total_paricles += 1;
          }

          // compute neighbours on demand
          bool has_neighbours = false;

          boost::hana::for_each(*bound_exprs[gi_a], [this, gi_a, ri_a,
                                                     &total_neighbours,
                                                     &has_neighbours,
                                                     &neighbours](auto &&arg) {
            if constexpr (boost::yap::is_expr<
                              remove_cvref_t<decltype(arg)>>::value) {
#ifdef PRTCL_DEBUG
              std::cout << " active only expr\n";
#endif
              // active-only expression
              boost::yap::transform(
                  std::forward<decltype(arg)>(arg),
                  eval_transform{ri_a, static_cast<size_t>(-1)});
            } else {
#ifdef PRTCL_DEBUG
              std::cout << " passive loop\n";
#endif
              for (size_t gi_p = 0; gi_p < this->get_group_count(); ++gi_p) {
                // skip group if not selected
                if (!arg[gi_p])
                  continue;

                // compute neighbours on demand
                if (!has_neighbours) {
                  for (auto &n : neighbours)
                    n.clear();

                  this->_grid.neighbours(
                      gi_a, ri_a, *this, [&neighbours](auto gr) {
                        neighbours[gr.get_group()].push_back(gr.get_index());
                      });

                  has_neighbours = true;
                }

                if (gi_a == 0) {
#pragma omp atomic
                  total_neighbours += neighbours[gi_p].size();
                }

                for (size_t ri_p : neighbours[gi_p]) {
#ifdef PRTCL_DEBUG
                  std::cout << "  p(" << gi_p << ", " << ri_p << ")\n";
#endif

                  boost::hana::for_each(*arg[gi_p], [ri_a, ri_p](auto &&arg) {
                    boost::yap::transform(std::forward<decltype(arg)>(arg),
                                          eval_transform{ri_a, ri_p});
                  });
                }
              }
            }
          });
        }
      }
    }

    std::cout << "total no. neighbours " << total_neighbours << "\n";
    std::cout << "total no. particles " << total_paricles << "\n";
    std::cout << "average no. neighbours "
              << (static_cast<long double>(total_neighbours) /
                  static_cast<long double>(total_paricles))
              << "\n";
  }

  template <typename Select, typename... Exprs>
  auto for_each_neighbour(Select &&select, Exprs &&... exprs) {
    // wrap the expressions into a passive loop
    return (expr::passive_loop<remove_cvref_t<Select>>{
        {std::forward<Select>(select)}})(std::forward<Exprs>(exprs)...);
  }

private:
  auto get_function(tag::dot) const {
    return [](auto &&lhs, auto &&rhs) {
      return math_traits::dot(std::forward<decltype(lhs)>(lhs),
                              std::forward<decltype(rhs)>(rhs));
    };
  }

  auto get_function(tag::norm) const {
    return [](auto &&arg) {
      return math_traits::norm(std::forward<decltype(arg)>(arg));
    };
  }

  auto get_function(tag::norm_squared) const {
    return [](auto &&arg) {
      return math_traits::norm_squared(std::forward<decltype(arg)>(arg));
    };
  }

  auto get_function(tag::normalized) const {
    return [](auto &&arg) {
      return math_traits::normalized(std::forward<decltype(arg)>(arg));
    };
  }

  auto get_function(tag::min) const {
    return [](auto &&... args) {
      return math_traits::min(std::forward<decltype(args)>(args)...);
    };
  }

  auto get_function(tag::max) const {
    return [](auto &&... args) {
      return math_traits::max(std::forward<decltype(args)>(args)...);
    };
  }

  auto get_function(tag::kernel) const {
    return [k = _kernel, h = _smoothing_scale](auto delta_x) {
      return k.eval(delta_x, h);
    };
  }

  auto get_function(tag::kernel_gradient) const {
    return [k = _kernel, h = _smoothing_scale](auto delta_x) {
      return k.evalgrad(delta_x, h);
    };
  }

private:
  Impl &impl() { return *static_cast<Impl *>(this); }
  Impl const &impl() const { return *static_cast<Impl const *>(this); }

private:
  scalar_type _smoothing_scale;
  kernel_type _kernel;

  grouped_uniform_grid<scalar_type, vector_extent> _grid;
}; // namespace prtcl

} // namespace prtcl

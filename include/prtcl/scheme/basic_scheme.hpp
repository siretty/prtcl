#pragma once

#include "../data/group_data.hpp"
#include "../data/host/grouped_uniform_grid.hpp"
#include "../expression/eval_context.hpp"
#include "../expression/transform/access_all_fields.hpp"
#include "../expression/transform/index_all_field_names.hpp"
#include "../expression/transform/resolve_all_fields.hpp"
#include "../math/kernel/cubic_spline_kernel.hpp"
#include "../math/traits/host_math_traits.hpp"

#include <vector>

#include <cstddef>

#include <boost/fusion/container/map.hpp>
#include <boost/fusion/tuple.hpp>

#include <eigen3/Eigen/Eigen>

namespace prtcl {

template <typename, size_t, template <typename> typename> class host_scheme;

template <typename Vector, typename VectorAccess> struct host_scheme_group {
  using vector_type = Vector;
  VectorAccess position;
};

template <typename VT, typename VA>
size_t get_element_count(host_scheme_group<VT, VA> const &);

template <typename VT, typename VA>
auto get_element_ref(host_scheme_group<VT, VA> const &);

template <typename T, size_t N,
          template <typename> typename KernelT = cubic_spline_kernel>
class host_scheme {
public:
  using math_traits = host_math_traits<T, N>;
  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;

public:
  using kernel_type = KernelT<math_traits>;
  using group_data_type = group_data<T, N>;

private:
  using group_buffer_type =
      typename result_of::get_buffer<group_data_type, tag::host>::type;

  auto functions() const {
    static auto the_functions = boost::fusion::as_map(boost::fusion::make_tuple(
        boost::fusion::make_pair<tag::dot>(dot_eval<math_traits>{}),
        boost::fusion::make_pair<tag::norm>(norm_eval<math_traits>{}),
        boost::fusion::make_pair<tag::norm_squared>(
            norm_squared_eval<math_traits>{}),
        boost::fusion::make_pair<tag::normalized>(
            normalized_eval<math_traits>{}),
        boost::fusion::make_pair<tag::min>(min_eval<math_traits>{}),
        boost::fusion::make_pair<tag::max>(max_eval<math_traits>{}),
        boost::fusion::make_pair<tag::kernel>(kernel_eval<kernel_type>{}),
        boost::fusion::make_pair<tag::kernel_gradient>(
            kernel_evalgrad<kernel_type>{})));
    return the_functions;
  }

private:
  template <typename Functions>
  using eval_context_t =
      expression::eval_context<scalar_type, vector_type, Functions>;

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

  size_t get_group_count() const { return group_data_.size(); }

public:
  void create_buffers() {
    // clear the buffer vector
    // TODO: assert on this
    group_buffer_.clear();

    // create a buffer object for each data object
    for (auto &gd : group_data_)
      group_buffer_.emplace_back(get_buffer(gd, tag::host{}));
  }

  void destroy_buffers() { group_buffer_.clear(); }

private:
  grouped_uniform_grid<T, N> grid_;

public:
  void set_neighbourhood_radius(scalar_type value) { grid_.set_radius(value); }

  void update_neighbourhoods() { grid_.update(*this); }

private:
  template <typename F> void _for_each_group(F &&f) {
#pragma omp parallel
    {
      // TODO: compute activation maps for the groups using the expression and
      // use
      //       that for iteration over the groups
      size_t const group_count = group_data_.size();
      for (size_t g_active = 0; g_active < group_count; ++g_active) {
        // std::cout << "  active group " << g_active << std::endl;
        std::invoke(std::forward<F>(f), g_active);
      }
    }
  }

  template <typename F> void _for_each_group_pair(F &&f) {
#pragma omp parallel
    {
      // TODO: compute activation maps for the groups using the expression and
      // use
      //       that for iteration over the groups
      size_t const group_count = group_data_.size();
      for (size_t g_active = 0; g_active < group_count; ++g_active) {
        for (size_t g_passive = 0; g_passive < group_count; ++g_passive) {
          // std::cout << "  active group " << g_active << " passive group "
          //          << g_passive << std::endl;
          std::invoke(std::forward<F>(f), g_active, g_passive);
        }
      }
    }
  }

  template <typename Exprs>
  void _for_each(Exprs const &exprs, size_t n_active) {
    // iterate over all particles
#pragma omp for
    for (size_t i_active = 0; i_active < n_active; ++i_active) {
      // std::cout << "    active index " << i_active << std::endl;
      // create the evaluation context and fill in particle indices
      eval_context_t<decltype(std::declval<host_scheme>().functions())> ctx;
      ctx.active = i_active;
      ctx.passive = i_active;
      ctx.functions = functions();
      // iterate over each expression e
      boost::fusion::for_each(exprs, [&ctx](auto const &e) {
        // evaluate the expression e
        boost::proto::eval(e, ctx);
      });
    }
  }

  template <typename Exprs>
  void _for_each_pair(Exprs const &exprs, size_t g_active, size_t g_passive) {
    size_t n_active = group_buffer_[g_active].size();
    // iterate over all particles
#pragma omp for
    for (size_t i_active = 0; i_active < n_active; ++i_active) {
      thread_local std::vector<size_t> neighbours;
      neighbours.clear();
      grid_.neighbours(g_active, i_active, *this, [g_passive](auto i_gr) {
        if (g_passive == i_gr.get_group())
          neighbours.push_back(i_gr.get_index());
      });
      for (size_t i_passive : neighbours) {
        // std::cout << "    active index " << i_active << " passive index "
        //          << i_passive << std::endl;
        // create the evaluation context and fill in particle indices
        eval_context_t<decltype(std::declval<host_scheme>().functions())> ctx;
        ctx.active = i_active;
        ctx.passive = i_passive;
        ctx.functions = functions();
        // iterate over each expression e
        boost::fusion::for_each(exprs, [&ctx](auto const &e) {
          // evaluate the expression e
          boost::proto::eval(e, ctx);
        });
      }
    }
  }

private:
  template <typename RawExpr, typename Groups>
  auto _transform_single_raw_expr(RawExpr &&raw_expr, Groups const &groups) {
    return expression::access_all_fields{}(
        expression::resolve_all_fields{}(expression::index_all_field_names{}(
                                             std::forward<RawExpr>(raw_expr)),
                                         0, groups),
        0, std::make_tuple());
  }

  template <typename Select, typename RawExpr>
  auto _transform_sigle_raw_expr_for_all_groups(Select &&select,
                                                RawExpr &&raw_expr) {
    struct groups_type {
      group_buffer_type const &active, &passive;
    };

    using expr_type = decltype(_transform_single_raw_expr(
        std::forward<RawExpr>(raw_expr), std::declval<groups_type>()));

    std::vector<std::vector<std::optional<expr_type>>> expr;
    expr.resize(group_buffer_.size());
    for (auto &e : expr)
      e.resize(group_buffer_.size());

    for (size_t g_active = 0; g_active < group_buffer_.size(); ++g_active) {
      for (size_t g_passive = 0; g_passive < group_buffer_.size();
           ++g_passive) {
        groups_type groups{group_buffer_[g_active], group_buffer_[g_passive]};
        if (std::invoke(std::forward<Select>(select), groups.active,
                        groups.passive)) {
          expr[g_active][g_passive] = _transform_single_raw_expr(
              std::forward<RawExpr>(raw_expr), groups);
        }
      }
    }

    return expr;
  }

  template <typename RawExprs, typename Groups>
  auto _transform_raw_exprs(RawExprs &&raw_exprs, Groups const &groups) {
    // TODO: (?) merge with other transforms into one line
    // combine field names and subscripted group indices
    auto indexed_exprs = boost::fusion::transform(
        raw_exprs, expression::index_all_field_names{});

    // TODO: (?) merge with other transforms into one line
    // resolve all field names to the group buffer
    auto resolved_exprs =
        boost::fusion::transform(indexed_exprs, [&groups](auto const &e) {
          return expression::resolve_all_fields{}(e, 0, groups);
        });

    // TODO: (?) merge with other transforms into one line
    // access all resolved field names
    return boost::fusion::transform(resolved_exprs, [](auto const &e) {
      return expression::access_all_fields{}(e, 0, std::make_tuple());
    });
  }

public:
  template <typename Select, typename... RawExprArgs>
  void for_each(Select &&select, RawExprArgs &&... raw_expr_args) {
    auto raw_exprs =
        boost::fusion::make_tuple(std::forward<RawExprArgs>(raw_expr_args)...);

    _for_each_group([this, &select, &raw_exprs](size_t g_active) {
      struct {
        group_buffer_type const &active, &passive;
      } groups{group_buffer_[g_active], group_buffer_[g_active]};

      if (std::invoke(std::forward<Select>(select), groups.active)) {
        auto exprs = this->_transform_raw_exprs(raw_exprs, groups);
        _for_each(exprs, groups.active.size());
      }
    });
  }

  template <typename Select, typename... RawExprArgs>
  void for_each_pair(Select &&select, RawExprArgs &&... raw_expr_args) {
    auto raw_exprs =
        boost::fusion::make_tuple(std::forward<RawExprArgs>(raw_expr_args)...);

    auto exprs =
        boost::fusion::transform(raw_exprs, [this, &select](auto const &e) {
          return this->_transform_sigle_raw_expr_for_all_groups(select, e);
        });

    std::vector<std::vector<size_t>> neighbours;

#pragma omp parallel private(neighbours)
    {
      for (size_t g_active = 0; g_active < group_buffer_.size(); ++g_active) {
        auto &active = group_buffer_[g_active];
#pragma omp for
        for (size_t i_active = 0; i_active < active.size(); ++i_active) {
          neighbours.resize(group_buffer_.size());
          for (auto &n : neighbours)
            n.clear();

          grid_.neighbours(g_active, i_active, *this, [&neighbours](auto i_gr) {
            neighbours[i_gr.get_group()].push_back(i_gr.get_index());
          });

          for (size_t g_passive = 0; g_passive < group_buffer_.size();
               ++g_passive) {
            for (size_t i_passive : neighbours[g_passive]) {
              eval_context_t<decltype(std::declval<host_scheme>().functions())>
                  ctx;
              ctx.active = i_active;
              ctx.passive = i_passive;
              ctx.functions = functions();
              // iterate over each expression e
              boost::fusion::for_each(
                  exprs, [g_active, g_passive, &ctx](auto const &e) {
                    if (e[g_active][g_passive]) {
                      // evaluate the expression e
                      boost::proto::eval(*e[g_active][g_passive], ctx);
                    }
                  });
            }
          }
        }
      }
    }
  }

  friend size_t get_group_count(host_scheme const &scheme) {
    return scheme.get_group_count();
  }

  friend auto get_group_ref(host_scheme const &scheme, size_t index) {
    auto x_access = get_ro_access(
        *scheme.group_buffer_[index].get_varying_vector("position"));
    return host_scheme_group<vector_type, decltype(x_access)>{x_access};
  }
};

template <typename VT, typename VA>
size_t get_element_count(host_scheme_group<VT, VA> const &g) {
  return g.position.size();
}

template <typename VT, typename VA>
auto get_element_ref(host_scheme_group<VT, VA> const &g, size_t index) {
  return g.position.template get<VT>(index);
}

template <typename T, size_t N, typename ExecutionTag> struct basic_scheme_impl;

template <typename T, size_t N>
struct basic_scheme_impl<T, N, tag::host> : public host_scheme<T, N> {};

template <typename T, size_t N, typename ExecutionTag>
class basic_scheme : public basic_scheme_impl<T, N, ExecutionTag> {
public:
  virtual ~basic_scheme() {}

  void execute() {
    this->create_buffers();
    this->execute_impl();
    this->destroy_buffers();
  }

  virtual void execute_impl() = 0;
};

} // namespace prtcl

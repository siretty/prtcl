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

  /*
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
  */
};

} // namespace prtcl

// ============================================================

#include "../data/host/grouped_uniform_grid.hpp"
#include "../expr/call.hpp"
#include "../expr/field.hpp"
#include "../expr/field_assign_transform.hpp"
#include "../expr/field_subscript_transform.hpp"
#include "../expr/field_value_transform.hpp"
#include "../expr/loop.hpp"

#include "../expr/text_transform.hpp"

#include <iostream>
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

  // needs_neighbours_transform {{{

private:
  struct needs_neighbours_transform {
    template <typename S, typename... Args>
    constexpr bool operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                              expr::loop<tag::passive, S>, Args &&...) const {
      return true;
    }

    template <typename... Args>
    constexpr bool
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               Args &&...) const {
      return false;
    }

    template <boost::yap::expr_kind Kind, typename... Args>
    constexpr bool operator()(boost::yap::expr_tag<Kind>,
                              Args &&... args) const {
      return (boost::yap::transform(
                  boost::yap::as_expr(std::forward<Args>(args)), *this) ||
              ...);
    }
  };

  // }}}

  // [inner,outer]_transform {{{

private:
  struct inner_transform {
    template <boost::yap::expr_kind Kind, typename... Args>
    void operator()(boost::yap::expr_tag<Kind>, Args &&... args) const {
      auto e = boost::yap::make_expression<Kind>(std::forward<Args>(args)...);

      // boost::yap::transform(e, expr::text_transform{std::cout});
      // std::cout << "\n";

      boost::yap::transform(e, expr::field_assign_transform{active, active});
    }

    template <typename S, typename... Args>
    void operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::loop<tag::passive, S> loop, Args &&... args) const {
      for (size_t gi_p = 0; gi_p < neighbours.size(); ++gi_p) {
        auto &gd_p = impl.get_group(gi_p);
        if (!loop.select(gd_p))
          continue;

        // std::cout << "passive group " << gi_p << "\n";

        for (size_t passive : neighbours[gi_p]) {
          // std::cout << "passive index " << passive << "\n";

          // non-debugging version:
          (boost::yap::transform(boost::yap::as_expr(std::forward<Args>(args)),
                                 expr::field_assign_transform{active, passive}),
           ...);
        }
      }
    }

    impl_type &impl;
    size_t active;
    std::vector<std::vector<size_t>> &neighbours;
  };

  struct outer_transform {
    template <typename S, typename... Args>
    void operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::loop<tag::active, S> loop, Args &&... args) const {
      bool needs_neighbours =
          (boost::yap::transform(boost::yap::as_expr(std::forward<Args>(args)),
                                 needs_neighbours_transform{}) ||
           ...);

      std::vector<std::vector<size_t>> neighbours;
#pragma omp parallel private(neighbours)
      {
        if (needs_neighbours) {
          neighbours.resize(impl.get_group_count());
        }

        for (size_t gi_a = 0; gi_a < impl.get_group_count(); ++gi_a) {
          auto &gd_a = impl.get_group(gi_a);
          if (!loop.select(gd_a))
            continue;

            // std::cout << "active group " << gi_a << "\n";

#pragma omp for
          for (size_t ri_a = 0; ri_a < gd_a.size(); ++ri_a) {
            // std::cout << "active index " << ri_a << "\n";

            if (needs_neighbours) {
              for (auto &n : neighbours)
                n.clear();

              // TODO: find neighbours, currently just all from the other group
              // std::cout << "finding neighbours\n";
              for (size_t gi_p = 0; gi_p < impl.get_group_count(); ++gi_p) {
                auto &gd_p = impl.get_group(gi_p);
                std::generate_n(std::back_inserter(neighbours[gi_p]),
                                gd_p.size(), [n = 0]() mutable { return n++; });
              }
            }

            (boost::yap::transform(
                 boost::yap::as_expr(std::forward<Args>(args)),
                 inner_transform{impl, ri_a, neighbours}),
             ...);
          }
        }
      }
    }

    impl_type &impl;
  };

  // }}}

  // call_transform {{{

private:
  struct call_transform {
    template <typename FT>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::call>,
                    expr::call<FT>) const {
      return boost::yap::as_expr(impl.get_function(FT{}));
    }

    impl_type &impl;
  };

  // }}}

  // buffer_transform {{{

  template <typename GetUniformAccess, typename GetVaryingAccess>
  struct access_transform {
    template <typename GT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, tag::scalar, GT, V> &&field_) const {
      auto access = std::invoke(
          get_uniform_access, impl.group_buffer_.get_uniform_scalars(),
          *impl.group_buffer_.get_uniform_scalar_index(field_.value));
      return expr::field_term<tag::uniform, tag::scalar, GT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
    }

    template <typename GT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::uniform, tag::vector, GT, V> &&field_) const {
      auto access = std::invoke(
          get_uniform_access, impl.group_buffer_.get_uniform_vectors(),
          *impl.group_buffer_.get_uniform_vector_index(field_.value));
      return expr::field_term<tag::uniform, tag::vector, GT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
    }

    template <typename GT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::varying, tag::scalar, GT, V> &&field_) const {
      auto access =
          std::invoke(get_varying_access,
                      *impl.group_buffer_.get_uniform_scalars(field_.value));
      return expr::field_term<tag::uniform, tag::scalar, GT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
    }

    template <typename GT, typename V>
    auto
    operator()(boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
               expr::field<tag::varying, tag::vector, GT, V> &&field_) const {
      auto access =
          std::invoke(get_varying_access,
                      *impl.group_buffer_.get_varying_vector(field_.value));
      return expr::field_term<tag::uniform, tag::vector, GT,
                              remove_cvref_t<decltype(access)>>{
          {std::move(access)}};
    }

    template <boost::yap::expr_kind Kind, typename... Args>
    auto operator()(boost::yap::expr_tag<Kind>, Args &&... args) const {
      return boost::yap::make_expression<Kind>(
          boost::yap::transform(boost::yap::as_expr(args), *this)...);
    }

    impl_type &impl;
    GetUniformAccess get_uniform_access;
    GetVaryingAccess get_varying_access;
  };

  template <typename GUA, typename GVA>
  auto make_access_transform(GUA &&gua, GVA &&gva) {
    return access_transform<remove_cvref_t<GUA>, remove_cvref_t<GVA>>{
        impl(), std::forward<GUA>(gua), std::forward<GVA>(gva)};
  }

  struct field_transform {
    template <typename LHS, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::assign>,
                    LHS &&lhs, RHS &&rhs) const {
      return boost::yap::make_expression<boost::yap::expr_kind::assign>(
          boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)),
                                make_rw_access_transform()),
          boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)),
                                make_ro_access_transform()));
    }

    template <typename LHS, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::plus_assign>,
                    LHS &&lhs, RHS &&rhs) const {
      return boost::yap::make_expression<boost::yap::expr_kind::plus_assign>(
          boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)),
                                make_rw_access_transform()),
          boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)),
                                make_ro_access_transform()));
    }

    template <typename LHS, typename RHS>
    auto operator()(boost::yap::expr_tag<boost::yap::expr_kind::minus_assign>,
                    LHS &&lhs, RHS &&rhs) const {
      return boost::yap::make_expression<boost::yap::expr_kind::minus_assign>(
          boost::yap::transform(boost::yap::as_expr(std::forward<LHS>(lhs)),
                                make_rw_access_transform()),
          boost::yap::transform(boost::yap::as_expr(std::forward<RHS>(rhs)),
                                make_ro_access_transform()));
    }

    template <typename Access> struct ro_uniform {
      auto get(size_t) const { return access.get(index); }

      Access access;
      size_t index;
    };

    template <typename Access> struct rw_uniform {
      template <typename Value> void set(size_t, Value &&value) const {
        access.set(index, std::forward<decltype(value)>(value));
      }

      auto get(size_t) const { return access.get(index); }

      Access access;
      size_t index;
    };

    auto make_rw_access_transform() const {
      return impl.make_access_transform(
          [](auto &&buffer, size_t index) {
            std::cout << "LOOKATME" << std::endl;
            auto access = get_rw_access(std::forward<decltype(buffer)>(buffer));
            return rw_uniform<remove_cvref_t<decltype(access)>>{
                std::move(access), index};
          },
          [](auto &&buffer) {
            return get_rw_access(std::forward<decltype(buffer)>(buffer));
          });
    }

    auto make_ro_access_transform() const {
      return impl.make_access_transform(
          [](auto &&buffer, size_t index) {
            auto access = get_ro_access(std::forward<decltype(buffer)>(buffer));
            return ro_uniform<remove_cvref_t<decltype(access)>>{
                std::move(access), index};
          },
          [](auto &&buffer) {
            return get_ro_access(std::forward<decltype(buffer)>(buffer));
          });
    }

    impl_type &impl;
  };

  // }}}

public:
  auto get_smoothing_scale() const { return _smoothing_scale; }

  void set_smoothing_scale(scalar_type value) {
    _smoothing_scale = value;
    _grid.set_radius(_kernel.get_support_radius(_smoothing_scale));
  }

protected:
  void update_neighbourhoods() { _grid.update(*this); }

protected:
  template <typename Select, typename... Exprs>
  void for_each(Select &&select, Exprs &&... exprs) {
    // wrap the expressions into an active loop
    auto e0 = (expr::active_loop<remove_cvref_t<Select>>{
        {std::forward<Select>(select)}})(std::forward<Exprs>(exprs)...);

    // bind field subscripts and calls
    auto e1 = boost::yap::transform(e0, expr::field_subscript_transform{},
                                    call_transform{impl()});

    //auto e2 = boost::yap::transform(e1, field_transform{impl()});

    // boost::yap::print(std::cout, e2);

    // boost::yap::transform(e1, expr::text_transform{std::cout});
    // std::cout << "\n";

    boost::yap::transform(e1, outer_transform{impl()});
  }

  template <typename Select, typename... Exprs>
  auto for_each_neighbour(Select &&select, Exprs &&... exprs) {
    // wrap the expressions into a passive loop
    auto e0 = (expr::passive_loop<remove_cvref_t<Select>>{
        {std::forward<Select>(select)}})(std::forward<Exprs>(exprs)...);

    return e0;
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
};

} // namespace prtcl

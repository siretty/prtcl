#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <variant>
#include <vector>

#include <cstddef>

#include <omp.h>

namespace prtcl::scheme {

template <typename T, size_t N> class prtcl_tests : private ::prtcl::model_base {
public:
  using scalar_type = typename ndfield_ref_t<T>::value_type;
  using vector_type = typename ndfield_ref_t<T, N>::value_type;
  using matrix_type = typename ndfield_ref_t<T, N, N>::value_type;

private:
  struct global_data { // {{{
    ndfield_ref_t<T> global;

  public:
    static void require(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      s_.add("global"_gsf);
    }

  public:
    void load(scheme_t<T, N> const &s_) {
      using namespace ::prtcl::expr_literals;
      global = s_.get("global"_gsf);
    }
  };
  // }}}

private:
  struct other_group_data { // {{{
    ndfield_ref_t<T, N> position;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("position"_vvf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      position = g_.get("position"_vvf);

      _size = g_.size();
    }
  };
  // }}}

private:
  struct main_group_data { // {{{
    ndfield_ref_t<T> uniform;
    ndfield_ref_t<T> varying;
    ndfield_ref_t<T, N> vv_a;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("uniform"_usf);
      g_.add("varying"_vsf);
      g_.add("vv_a"_vvf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      uniform = g_.get("uniform"_usf);
      varying = g_.get("varying"_vsf);
      vv_a = g_.get("vv_a"_vvf);

      _size = g_.size();
    }
  };
  // }}}

private:
  size_t _group_count;
  global_data _global;
  std::vector<other_group_data> _groups_other;
  std::vector<main_group_data> _groups_main;
  std::vector<std::vector<std::vector<size_t>>> _per_thread_neighbours;

public:
  static void require(scheme_t<T, N> &s_) { // {{{
    global_data::require(s_);
    
    for (auto &group : s_.groups()) {
      if (group.get_type() == "other")
        other_group_data::require(group);
      if (group.get_type() == "main")
        main_group_data::require(group);
    }
  }
  // }}}

public:
  void load(scheme_t<T, N> const &s_) { // {{{
    _group_count = s_.get_group_count();

    _global.load(s_);

    for (auto [i, n, group] : s_.enumerate_groups()) {
      if (group.get_type() == "other") {
        auto &data = _groups_other.emplace_back();
        data.load(group);
        data._index = i;
      }
      if (group.get_type() == "main") {
        auto &data = _groups_main.emplace_back();
        data.load(group);
        data._index = i;
      }
    }
  }
  // }}}

public:
  template <typename NHood>
  void reduce_sum(NHood const &nhood_) {
    (void)(nhood_);

    auto &g = _global;
    (void)(g);

    { // foreach particle {{{
      std::vector<scalar_type> per_thread_rd_gs_global;
      std::vector<scalar_type> per_thread_rd_us_uniform;

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
          per_thread_rd_gs_global.resize(thread_count);
          per_thread_rd_us_uniform.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        // select and resize the neighbour storage for the current thread
        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // reserve space for neighbours of each group
        for (auto &pgn : neighbours)
          pgn.reserve(100);

        // per-thread reduction variables
        auto &rd_gs_global = per_thread_rd_gs_global[thread_index];
        auto &rd_us_uniform = per_thread_rd_us_uniform[thread_index];

        // initialize global reduction variables
        rd_gs_global = 0;
        #pragma omp single
        {
          g.global[0] = 0;
        }

        for (auto &p : _groups_main) {
          // initialize uniform reduction variables
          rd_us_uniform = 0;
          #pragma omp single
          {
            p.uniform[0] = 0;
          }

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            for (auto &pgn : neighbours)
              pgn.clear();

            bool has_neighbours = false;
            (void)(has_neighbours);

            rd_us_uniform += ( p.varying[i] / p._size );

            rd_gs_global += ( p.varying[i] / p._size );
          } // i

          // combine reduction variables
          #pragma omp critical
          {
            p.uniform[0] += rd_us_uniform;
          }
        } // p

        // combine reduction variables
        #pragma omp critical
        {
          g.global[0] += rd_gs_global;
        }
      } // omp parallel
    } // }}}
  }

public:
  template <typename NHood>
  void reduce_sum_neighbours(NHood const &nhood_) {
    (void)(nhood_);

    auto &g = _global;
    (void)(g);

    { // foreach particle {{{
      std::vector<scalar_type> per_thread_rd_gs_global;
      std::vector<scalar_type> per_thread_rd_us_uniform;

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
          per_thread_rd_gs_global.resize(thread_count);
          per_thread_rd_us_uniform.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        // select and resize the neighbour storage for the current thread
        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // reserve space for neighbours of each group
        for (auto &pgn : neighbours)
          pgn.reserve(100);

        // per-thread reduction variables
        auto &rd_gs_global = per_thread_rd_gs_global[thread_index];
        auto &rd_us_uniform = per_thread_rd_us_uniform[thread_index];

        // initialize global reduction variables
        rd_gs_global = 0;
        #pragma omp single
        {
          g.global[0] = 0;
        }

        for (auto &p : _groups_main) {
          // initialize uniform reduction variables
          rd_us_uniform = 0;
          #pragma omp single
          {
            p.uniform[0] = 0;
          }

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            for (auto &pgn : neighbours)
              pgn.clear();

            bool has_neighbours = false;
            (void)(has_neighbours);

            p.vv_a[i] = vector_type::Zero();

            if (!has_neighbours) {
              nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
                neighbours[n_index].push_back(j);
              });
              has_neighbours = true;
            }

            for (auto &n : _groups_other) {
              for (size_t j : neighbours[n._index]) {
                p.vv_a[i] += n.position[j];
                rd_gs_global += 1;
              } // j
            } // n

            rd_us_uniform += ( p.vv_a[i] ).matrix().norm();
          } // i

          // combine reduction variables
          #pragma omp critical
          {
            p.uniform[0] += rd_us_uniform;
          }
        } // p

        // combine reduction variables
        #pragma omp critical
        {
          g.global[0] += rd_gs_global;
        }
      } // omp parallel
    } // }}}
  }

public:
  template <typename NHood>
  void call_norm(NHood const &nhood_) {
    (void)(nhood_);

    auto &g = _global;
    (void)(g);

    { // foreach particle {{{

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        // select and resize the neighbour storage for the current thread
        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // reserve space for neighbours of each group
        for (auto &pgn : neighbours)
          pgn.reserve(100);

        for (auto &p : _groups_main) {

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            for (auto &pgn : neighbours)
              pgn.clear();

            bool has_neighbours = false;
            (void)(has_neighbours);

            p.varying[i] = ( p.vv_a[i] ).matrix().norm();
          } // i
        } // p
      } // omp parallel
    } // }}}
  }

public:
  static std::string_view xml() { // {{{
    return R"prtcl(
<prtcl>
<section name="reduce_sum">
  <particle_loop>
    <selector>
      <rd op="plus">
        <field kind="uniform" type="scalar">uniform</field>
        <binary op="/">
          <subscript>
            <field kind="varying" type="scalar">varying</field>
            <group>active</group>
          </subscript>
          <call name="particle_count">
          </call>
        </binary>
      </rd>
      <rd op="plus">
        <field kind="global" type="scalar">global</field>
        <binary op="/">
          <subscript>
            <field kind="varying" type="scalar">varying</field>
            <group>active</group>
          </subscript>
          <call name="particle_count">
          </call>
        </binary>
      </rd>
    </selector>
  </particle_loop>
</section>
<section name="reduce_sum_neighbours">
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="vector">vv_a</field>
            <group>active</group>
          </subscript>
          <call name="zero_vector">
          </call>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="vector">vv_a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="vector">position</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
          <rd op="plus">
            <field kind="global" type="scalar">global</field>
            <value>1</value>
          </rd>
        </selector>
      </neighbour_loop>
      <rd op="plus">
        <field kind="uniform" type="scalar">uniform</field>
        <call name="norm">
          <subscript>
            <field kind="varying" type="vector">vv_a</field>
            <group>active</group>
          </subscript>
        </call>
      </rd>
    </selector>
  </particle_loop>
</section>
<section name="call_norm">
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">varying</field>
            <group>active</group>
          </subscript>
          <call name="norm">
            <subscript>
              <field kind="varying" type="vector">vv_a</field>
              <group>active</group>
            </subscript>
          </call>
        </assign>
      </eq>
    </selector>
  </particle_loop>
</section>
</prtcl>
)prtcl";
  }
  // }}}
};

} // namespace prtcl::scheme

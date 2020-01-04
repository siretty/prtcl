#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <variant>
#include <vector>

#include <cstddef>

#include <omp.h>

namespace prtcl::scheme {

template <typename T, size_t N> class TEST : private ::prtcl::model_base {
public:
  using scalar_type = typename ndfield_ref_t<T>::value_type;
  using vector_type = typename ndfield_ref_t<T, N>::value_type;
  using matrix_type = typename ndfield_ref_t<T, N, N>::value_type;

private:
  struct global_data { // {{{
    ndfield_ref_t<T> gs;

  public:
    static void require(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      s_.add("gs"_gsf);
    }

  public:
    void load(scheme_t<T, N> const &s_) {
      using namespace ::prtcl::expr_literals;
      gs = s_.get("gs"_gsf);
    }
  };
  // }}}

private:
  struct boundary_group_data { // {{{
    ndfield_ref_t<T> b;
    ndfield_ref_t<T> c;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("b"_vsf);
      g_.add("c"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      b = g_.get("b"_vsf);
      c = g_.get("c"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  struct fluid_group_data { // {{{
    ndfield_ref_t<T> d;
    ndfield_ref_t<T> a;
    ndfield_ref_t<T> b;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("d"_usf);
      g_.add("a"_vsf);
      g_.add("b"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      d = g_.get("d"_usf);
      a = g_.get("a"_vsf);
      b = g_.get("b"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  size_t _group_count;
  global_data _global;
  std::vector<boundary_group_data> _groups_boundary;
  std::vector<fluid_group_data> _groups_fluid;
  std::vector<std::vector<std::vector<size_t>>> _per_thread_neighbours;

public:
  static void require(scheme_t<T, N> &s_) { // {{{
    global_data::require(s_);

    for (auto &group : s_.groups()) {
      if (group.get_type() == "boundary")
        boundary_group_data::require(group);
      if (group.get_type() == "fluid")
        fluid_group_data::require(group);
    }
  }
  // }}}

public:
  void load(scheme_t<T, N> const &s_) { // {{{
    _group_count = s_.get_group_count();

    _global.load(s_);

    for (auto [i, n, group] : s_.enumerate_groups()) {
      if (group.get_type() == "boundary") {
        auto &data = _groups_boundary.emplace_back();
        data.load(group);
        data._index = i;
      }
      if (group.get_type() == "fluid") {
        auto &data = _groups_fluid.emplace_back();
        data.load(group);
        data._index = i;
      }
    }
  }
  // }}}

public:
  template <typename NHood>
  void section_a(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] = 0;

    { // foreach particle {{{
      std::vector<scalar_type> per_thread_rd_gs_gs;
      std::vector<scalar_type> per_thread_rd_us_d;

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
          per_thread_rd_gs_gs.resize(thread_count);
          per_thread_rd_us_d.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        // select and resize the neighbour storage for the current thread
        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // reserve space for neighbours of each group
        for (auto &pgn : neighbours)
          pgn.reserve(100);

        // per-thread reduction variables
        auto &rd_gs_gs = per_thread_rd_gs_gs[thread_index];
        auto &rd_us_d = per_thread_rd_us_d[thread_index];

        // initialize global reduction variables
        rd_gs_gs = 0;
        #pragma omp single
        {
          g.gs[0] = 0;
        }

        for (auto &p : _groups_fluid) {
          // initialize uniform reduction variables
          rd_us_d = 0;
          #pragma omp single
          {
            p.d[0] = 0;
          }

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            for (auto &pgn : neighbours)
              pgn.clear();

            bool has_neighbours = false;

            p.a[i] = 0;

            if (!has_neighbours) {
              nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
                neighbours[n_index].push_back(j);
              });
              has_neighbours = true;
            }

            for (auto &n : _groups_boundary) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.c[j];
                rd_us_d += n.b[j];
              } // j
            } // n

            for (auto &n : _groups_fluid) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.b[j];
                rd_us_d += n.b[j];
                rd_gs_gs -= 1;
              } // j
            } // n
          } // i

          // combine reduction variables
          #pragma omp critical
          {
            p.d[0] += rd_us_d;
          }
        } // p

        // combine reduction variables
        #pragma omp critical
        {
          g.gs[0] -= rd_gs_gs;
        }
      } // omp parallel
    } // }}}

    g.gs[0] += 1;
  }

public:
  template <typename NHood>
  void section_b(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] += 2;

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

        for (auto &p : _groups_fluid) {

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            for (auto &pgn : neighbours)
              pgn.clear();

            bool has_neighbours = false;

            p.a[i] = 0;

            if (!has_neighbours) {
              nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
                neighbours[n_index].push_back(j);
              });
              has_neighbours = true;
            }

            for (auto &n : _groups_boundary) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.c[j];
              } // j
            } // n

            for (auto &n : _groups_fluid) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.b[j];
              } // j
            } // n
          } // i
        } // p
      } // omp parallel
    } // }}}

    g.gs[0] += 3;
  }

public:
  static std::string_view xml() { // {{{
    return R"prtcl(
<prtcl>
<section name="section_a">
  <eq>
    <assign>
      <field kind="global" type="scalar">gs</field>
      <value>0</value>
    </assign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
          <rd op="plus">
            <field kind="uniform" type="scalar">d</field>
            <subscript>
              <field kind="varying" type="scalar">b</field>
              <group>passive</group>
            </subscript>
          </rd>
          <rd op="minus">
            <field kind="global" type="scalar">gs</field>
            <value>1</value>
          </rd>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
          <rd op="plus">
            <field kind="uniform" type="scalar">d</field>
            <subscript>
              <field kind="varying" type="scalar">b</field>
              <group>passive</group>
            </subscript>
          </rd>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>1</value>
    </opassign>
  </eq>
</section>
<section name="section_b">
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>2</value>
    </opassign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>3</value>
    </opassign>
  </eq>
</section>
</prtcl>
)prtcl";
  }
  // }}}
};

} // namespace prtcl::scheme

/*
#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <variant>
#include <vector>

#include <cstddef>

#include <omp.h>

namespace prtcl::scheme {

template <typename T, size_t N> class TEST : private ::prtcl::model_base {
public:
  using scalar_type = typename ndfield_ref_t<T>::value_type;
  using vector_type = typename ndfield_ref_t<T, N>::value_type;
  using matrix_type = typename ndfield_ref_t<T, N, N>::value_type;

private:
  struct global_data { // {{{
    ndfield_ref_t<T> gs;

  public:
    static void require(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      s_.add("gs"_gsf);
    }

  public:
    void load(scheme_t<T, N> const &s_) {
      using namespace ::prtcl::expr_literals;
      gs = s_.get("gs"_gsf);
    }
  };
  // }}}

private:
  struct boundary_group_data { // {{{
    ndfield_ref_t<T> b;
    ndfield_ref_t<T> c;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("b"_vsf);
      g_.add("c"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      b = g_.get("b"_vsf);
      c = g_.get("c"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  struct fluid_group_data { // {{{
    ndfield_ref_t<T> d;
    ndfield_ref_t<T> a;
    ndfield_ref_t<T> b;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("d"_usf);
      g_.add("a"_vsf);
      g_.add("b"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      d = g_.get("d"_usf);
      a = g_.get("a"_vsf);
      b = g_.get("b"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  size_t _group_count;
  global_data _global;
  std::vector<boundary_group_data> _groups_boundary;
  std::vector<fluid_group_data> _groups_fluid;
  std::vector<std::vector<std::vector<size_t>>> _per_thread_neighbours;

public:
  static void require(scheme_t<T, N> &s_) { // {{{
    global_data::require(s_);

    for (auto &group : s_.groups()) {
      if (group.get_type() == "boundary")
        boundary_group_data::require(group);
      if (group.get_type() == "fluid")
        fluid_group_data::require(group);
    }
  }
  // }}}

public:
  void load(scheme_t<T, N> const &s_) { // {{{
    _group_count = s_.get_group_count();

    _global.load(s_);

    for (auto [i, n, group] : s_.enumerate_groups()) {
      if (group.get_type() == "boundary") {
        auto &data = _groups_boundary.emplace_back();
        data.load(group);
        data._index = i;
      }
      if (group.get_type() == "fluid") {
        auto &data = _groups_fluid.emplace_back();
        data.load(group);
        data._index = i;
      }
    }
  }
  // }}}

public:
  template <typename NHood>
  void section_a(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] = 0;

    { // foreach particle {{{
      std::vector<scalar_type> per_thread_rd_gs_gs;
      std::vector<scalar_type> per_thread_rd_us_d;

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
          per_thread_rd_gs_gs.resize(thread_count);
          per_thread_rd_us_d.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // per-thread reduction variables
        auto &rd_gs_gs = per_thread_rd_gs_gs[thread_index];
        auto &rd_us_d = per_thread_rd_us_d[thread_index];

        // initialize global reduction variables
        rd_gs_gs = 0;
        #pragma omp single
        {
          g.gs[0] = 0;
        }

        for (auto &p : _groups_fluid) {
          // initialize uniform reduction variables
          rd_us_d = 0;
          #pragma omp single
          {
            p.d[0] = 0;
          }

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            bool has_neighbours = false;
            for (auto &pgn : neighbours) {
              pgn.clear();
              pgn.reserve(100);
            }

            p.a[i] = 0;

            if (!has_neighbours) {
              nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
                neighbours[n_index].push_back(j);
              });
            }

            for (auto &n : _groups_boundary) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.c[j];
                rd_us_d += n.b[j];
              } // j
            } // n

            for (auto &n : _groups_fluid) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.b[j];
                rd_us_d += n.b[j];
                rd_gs_gs -= 1;
              } // j
            } // n
          } // i

          // combine reduction variables
          #pragma omp critical
          {
            p.d[0] += rd_us_d;
          }
        } // p

        // combine reduction variables
        #pragma omp critical
        {
          g.gs[0] -= rd_gs_gs;
        }
      } // omp parallel
    } // }}}

    g.gs[0] += 1;
  }

public:
  template <typename NHood>
  void section_b(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] += 2;

    { // foreach particle {{{

      #pragma omp parallel
      {
        #pragma omp single
        {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread_neighbours.resize(thread_count);
        }

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        auto &neighbours = _per_thread_neighbours[thread_index];
        neighbours.resize(_group_count);

        // per-thread reduction variables

        // initialize global reduction variables

        for (auto &p : _groups_fluid) {
          // initialize uniform reduction variables

          #pragma omp for
          for (size_t i = 0; i < p._size; ++i) {
            bool has_neighbours = false;
            for (auto &pgn : neighbours) {
              pgn.clear();
              pgn.reserve(100);
            }

            p.a[i] = 0;

            if (!has_neighbours) {
              nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
                neighbours[n_index].push_back(j);
              });
            }

            for (auto &n : _groups_boundary) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.c[j];
              } // j
            } // n

            for (auto &n : _groups_fluid) {
              for (size_t j : neighbours[n._index]) {
                p.a[i] += n.b[j];
              } // j
            } // n
          } // i

          // combine reduction variables
          #pragma omp critical
          {
          }
        } // p

        // combine reduction variables
        #pragma omp critical
        {
        }
      } // omp parallel
    } // }}}

    g.gs[0] += 3;
  }

public:
  static std::string_view xml() { // {{{
    return R"prtcl(
<prtcl>
<section name="section_a">
  <eq>
    <assign>
      <field kind="global" type="scalar">gs</field>
      <value>0</value>
    </assign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
          <rd op="plus">
            <field kind="uniform" type="scalar">d</field>
            <subscript>
              <field kind="varying" type="scalar">b</field>
              <group>passive</group>
            </subscript>
          </rd>
          <rd op="minus">
            <field kind="global" type="scalar">gs</field>
            <value>1</value>
          </rd>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
          <rd op="plus">
            <field kind="uniform" type="scalar">d</field>
            <subscript>
              <field kind="varying" type="scalar">b</field>
              <group>passive</group>
            </subscript>
          </rd>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>1</value>
    </opassign>
  </eq>
</section>
<section name="section_b">
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>2</value>
    </opassign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>3</value>
    </opassign>
  </eq>
</section>
</prtcl>
)prtcl";
  }
  // }}}
};

} // namespace prtcl::scheme

#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <variant>
#include <vector>

#include <cstddef>

#include <omp.h>

namespace prtcl::scheme {

template <typename T, size_t N> class TEST : private ::prtcl::model_base {
public:
  using scalar_type = typename ndfield_ref_t<T>::value_type;
  using vector_type = typename ndfield_ref_t<T, N>::value_type;
  using matrix_type = typename ndfield_ref_t<T, N, N>::value_type;

private:
  struct global_data { // {{{
    ndfield_ref_t<T> gs;

  public:
    static void require(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      s_.add("gs"_gsf);
    }

  public:
    void load(scheme_t<T, N> const &s_) {
      using namespace ::prtcl::expr_literals;
      gs = s_.get("gs"_gsf);
    }
  };
  // }}}

private:
  struct boundary_group_data { // {{{
    ndfield_ref_t<T> c;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("c"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      c = g_.get("c"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  struct fluid_group_data { // {{{
    ndfield_ref_t<T> a;
    ndfield_ref_t<T> b;

    size_t _size;
    size_t _index;

  public:
    static void require(group_t<T, N> &g_) {
      using namespace ::prtcl::expr_literals;
      g_.add("a"_vsf);
      g_.add("b"_vsf);
    }

  public:
    void load(group_t<T, N> const &g_) {
      using namespace ::prtcl::expr_literals;
      a = g_.get("a"_vsf);
      b = g_.get("b"_vsf);

      _size = g_.size();
    }
  };
  // }}}

private:
  size_t _group_count;
  global_data _global;
  std::vector<boundary_group_data> _groups_boundary;
  std::vector<fluid_group_data> _groups_fluid;
  std::vector<std::vector<std::vector<size_t>>> _per_thread_neighbours;

public:
  static void require(scheme_t<T, N> &s_) { // {{{
    global_data::require(s_);

    for (auto &group : s_.groups()) {
      if (group.get_type() == "boundary")
        boundary_group_data::require(group);
      if (group.get_type() == "fluid")
        fluid_group_data::require(group);
    }
  }
  // }}}

public:
  void load(scheme_t<T, N> const &s_) { // {{{
    _group_count = s_.get_group_count();

    _global.load(s_);

    for (auto [i, n, group] : s_.enumerate_groups()) {
      if (group.get_type() == "boundary") {
        auto &data = _groups_boundary.emplace_back();
        data.load(group);
        data._index = i;
      }
      if (group.get_type() == "fluid") {
        auto &data = _groups_fluid.emplace_back();
        data.load(group);
        data._index = i;
      }
    }
  }
  // }}}

public:
  template <typename NHood>
  void section_a(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] = 0;

    #pragma omp parallel
    {
      #pragma omp single
      {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());
        _per_thread_neighbours.resize(thread_count);
      }

      auto &neighbours = _per_thread_neighbours[static_cast<size_t>(omp_get_thread_num())];
      neighbours.resize(_group_count);

      for (auto &p : _groups_fluid) {
        #pragma omp for
        for (size_t i = 0; i < p._size; ++i) {
          bool has_neighbours = false;
          for (auto &pgn : neighbours) {
            pgn.clear();
            pgn.reserve(100);
          }

          p.a[i] = 0;

          if (!has_neighbours) {
            nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
              neighbours[n_index].push_back(j);
            });
          }

          for (auto &n : _groups_boundary) {
            for (size_t j : neighbours[n._index]) {
              p.a[i] += n.c[j];
            } // j
          } // n

          for (auto &n : _groups_fluid) {
            for (size_t j : neighbours[n._index]) {
              p.a[i] += n.b[j];
            } // j
          } // n
        } // i
      } // p
    }

    g.gs[0] += 1;
  }

public:
  template <typename NHood>
  void section_b(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] += 2;

    #pragma omp parallel
    {
      #pragma omp single
      {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());
        _per_thread_neighbours.resize(thread_count);
      }

      auto &neighbours = _per_thread_neighbours[static_cast<size_t>(omp_get_thread_num())];
      neighbours.resize(_group_count);

      for (auto &p : _groups_fluid) {
        #pragma omp for
        for (size_t i = 0; i < p._size; ++i) {
          bool has_neighbours = false;
          for (auto &pgn : neighbours) {
            pgn.clear();
            pgn.reserve(100);
          }

          p.a[i] = 0;

          if (!has_neighbours) {
            nhood_.neighbours(p._index, i, [&neighbours](auto n_index, auto j) {
              neighbours[n_index].push_back(j);
            });
          }

          for (auto &n : _groups_boundary) {
            for (size_t j : neighbours[n._index]) {
              p.a[i] += n.c[j];
            } // j
          } // n

          for (auto &n : _groups_fluid) {
            for (size_t j : neighbours[n._index]) {
              p.a[i] += n.b[j];
            } // j
          } // n
        } // i
      } // p
    }

    g.gs[0] += 3;
  }

public:
  static std::string_view xml() { // {{{
    return R"prtcl(
<prtcl>
<section name="section_a">
  <eq>
    <assign>
      <field kind="global" type="scalar">gs</field>
      <value>0</value>
    </assign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>1</value>
    </opassign>
  </eq>
</section>
<section name="section_b">
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>2</value>
    </opassign>
  </eq>
  <particle_loop>
    <selector>
      <eq>
        <assign>
          <subscript>
            <field kind="varying" type="scalar">a</field>
            <group>active</group>
          </subscript>
          <value>0</value>
        </assign>
      </eq>
      <neighbour_loop>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">b</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
        <selector>
          <eq>
            <opassign op="+=">
              <subscript>
                <field kind="varying" type="scalar">a</field>
                <group>active</group>
              </subscript>
              <subscript>
                <field kind="varying" type="scalar">c</field>
                <group>passive</group>
              </subscript>
            </opassign>
          </eq>
        </selector>
      </neighbour_loop>
    </selector>
  </particle_loop>
  <eq>
    <opassign op="+=">
      <field kind="global" type="scalar">gs</field>
      <value>3</value>
    </opassign>
  </eq>
</section>
</prtcl>
)prtcl";
  }
  // }}}
};

} // namespace prtcl::scheme
*/

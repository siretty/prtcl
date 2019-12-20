#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <variant>
#include <vector>

#include <cstddef>

namespace prtcl::scheme {

template <typename T, size_t N> class TEST : private ::prtcl::model_base {
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

    size_t _index;
    size_t _size;

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

    size_t _index;
    size_t _size;

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

  using group_data =
      std::variant<std::monostate, boundary_group_data, fluid_group_data>;

private:
  std::vector<boundary_group_data> _groups_boundary;
  std::vector<fluid_group_data> _groups_fluid;
  global_data _global;
  std::vector<group_data> _groups;
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
    _global.load(s_);

    _groups.resize(s_.get_group_count());
    for (auto [i, n, group] : s_.enumerate_groups()) {
      if (group.get_type() == "boundary") {
        std::visit(
            ::prtcl::meta::overload{
                [](std::monostate) {},
                [&group = group](auto &data) { data.load(group); }},
            _groups[i] = boundary_group_data{});
        auto &data = _groups_boundary.emplace_back();
        data.load(group);
        data._index = i;
      }
      if (group.get_type() == "fluid") {
        std::visit(
            ::prtcl::meta::overload{
                [](std::monostate) {},
                [&group = group](auto &data) { data.load(group); }},
            _groups[i] = fluid_group_data{});
        auto &data = _groups_fluid.emplace_back();
        data.load(group);
        data._index = i;
      }
    }
  }
  // }}}

public:
  template <typename NHood> void Test(NHood const &nhood_) {
    auto &g = _global;

    g.gs[0] = 0;

#pragma omp parallel
    {
#pragma omp single
      {
        _per_thread_neighbours.resize(
            static_cast<size_t>(omp_get_num_threads()));
      }

      auto &neighbours =
          _per_thread_neighbours[static_cast<size_t>(omp_get_thread_num())];
      neighbours.resize(_groups.size());

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
            }
          }
          for (auto &n : _groups_fluid) {
            for (size_t j : neighbours[n._index]) {
              p.a[i] += n.b[j];
            }
          }
        }
      }
    }

    g.gs[0] += 1;
  }

public:
  static std::string_view xml() { // {{{
    return R"prtcl(
<prtcl>
<section name="Test">
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
</prtcl>
)prtcl";
  }
  // }}}
};

} // namespace prtcl::scheme

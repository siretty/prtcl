#pragma once

#include <prtcl/model_base.hpp>

#include <string_view>
#include <vector>

#include <cstddef>

namespace prtcl::scheme {

template <typename T, size_t N> class TEST : private ::prtcl::model_base {
  struct {
    ndfield_ref_t<T> gs;

  public:
    static void require(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      s_.add("gs"_gsf);
    }

  public:
    void load(scheme_t<T, N> &s_) {
      using namespace ::prtcl::expr_literals;
      gs = s_.get("gs"_gsf);
    }
  } g;

public:
  struct {
    struct boundary_group_type {
      ndfield_ref_t<T> c;

      size_t _size;

    public:
      static void require(group_t<T, N> &g_) {
        using namespace ::prtcl::expr_literals;
        g_.add("c"_vsf);
      }

    public:
      void load(group_t<T, N> &g_) {
        using namespace ::prtcl::expr_literals;
        c = g_.get("c"_vsf);

        _size = g_.size();
      }
    };

    std::vector<boundary_group_type> boundary;

    struct fluid_group_type {
      ndfield_ref_t<T> a;
      ndfield_ref_t<T> b;

      size_t _size;

    public:
      static void require(group_t<T, N> &g_) {
        using namespace ::prtcl::expr_literals;
        g_.add("a"_vsf);
        g_.add("b"_vsf);
      }

    public:
      void load(group_t<T, N> &g_) {
        using namespace ::prtcl::expr_literals;
        a = g_.get("a"_vsf);
        b = g_.get("b"_vsf);

        _size = g_.size();
      }
    };

    std::vector<fluid_group_type> fluid;

  } _groups;

public:
  static void require(scheme_t<T, N> &s_) {
    decltype(g)::require(s_);

    for (auto &group : s_.groups()) {
      if (group.get_type() == "boundary") {
        decltype(_groups)::boundary_group_type::require(group);
      }
    }

    for (auto &group : s_.groups()) {
      if (group.get_type() == "fluid") {
        decltype(_groups)::fluid_group_type::require(group);
      }
    }
  }

public:
  void load(scheme_t<T, N> &s_) {
    g.load(s_);

    _groups.boundary.clear();
    for (auto &group : s_.groups()) {
      if (group.get_type() == "boundary") {
        _groups.boundary.emplace_back().load(group);
      }
    }

    _groups.fluid.clear();
    for (auto &group : s_.groups()) {
      if (group.get_type() == "fluid") {
        _groups.fluid.emplace_back().load(group);
      }
    }
  }

  void Test() {
    g.gs[0] = 0;
#pragma omp parallel
    {
      for (auto &p : _groups.fluid) {
#pragma omp for
        for (size_t i = 0; i < p._size; ++i) {
          p.a[i] = 0;
          for (auto &n : _groups.fluid) {
            for (size_t j = 0; j < n._size; ++j) {
              p.a[i] += n.b[j];
            }
          }
          for (auto &n : _groups.boundary) {
            for (size_t j = 0; j < n._size; ++j) {
              p.a[i] += n.c[j];
            }
          }
        }
      }
    }
    g.gs[0] += 1;
  }

  static std::string_view xml() {
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
};

} // namespace prtcl::scheme

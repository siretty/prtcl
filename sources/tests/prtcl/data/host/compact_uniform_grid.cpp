#include <catch.hpp>

#include <prtcl/data/host/compact_uniform_grid.hpp>

#include <array>
#include <iostream>
#include <vector>

#include <eigen3/Eigen/Eigen>

namespace {

template <typename T, size_t N> struct group {
  std::vector<Eigen::Matrix<T, N, 1>> position;

  friend size_t get_element_count(group const &g) { return g.position.size(); }

  friend auto const &get_element_ref(group const &g, size_t i) {
    return g.position[i];
  }
};

template <typename T, size_t N> struct multiple_groups {
  std::vector<group<T, N>> groups;

  friend size_t get_group_count(multiple_groups const &g) {
    return g.groups.size();
  }

  friend auto const &get_group_ref(multiple_groups const &g, size_t i) {
    return g.groups[i];
  }
};

} // namespace

TEST_CASE("prtcl/data/host/compact_uniform_grid",
          "[prtcl][data][host][compact_uniform_grid]") {
  using T = float;
  constexpr size_t N = 3;

  multiple_groups<T, N> mg;

  mg.groups.resize(3);

  {
    auto &g = mg.groups[0];

    g.position.push_back({+1, 0, 0});
    g.position.push_back({0, +1, 0});
    g.position.push_back({0, 0, +1});

    g.position.push_back({+2, 0, 0});
    g.position.push_back({0, +2, 0});
    g.position.push_back({0, 0, +2});

    g.position.push_back({+3, 0, 0});
    g.position.push_back({0, +3, 0});
    g.position.push_back({0, 0, +3});
  }

  {
    auto &g = mg.groups[1];

    g.position.push_back({-1, 0, 0});
    g.position.push_back({0, -1, 0});
    g.position.push_back({0, 0, -1});

    g.position.push_back({-2, 0, 0});
    g.position.push_back({0, -2, 0});
    g.position.push_back({0, 0, -2});

    g.position.push_back({-3, 0, 0});
    g.position.push_back({0, -3, 0});
    g.position.push_back({0, 0, -3});
  }

  {
    auto &g = mg.groups[2];

    g.position.push_back({0, 0, 0});
  }

  prtcl::compact_uniform_grid<T, N> grid;
  grid.set_radius(1.5f);
  grid.initialize(mg);

  for (size_t step = 0; step < 10; ++step) {
    std::cout << "step #" << step << "\n";

    for (auto &g : mg.groups) {
      for (auto &p : g.position) {
        p += p.Constant(0.1f);
      }
    }

    grid.update(mg);
    grid.neighbours(2, 0, mg, [&mg](auto gi, auto ri) {
      std::cout << gi << " " << ri << "[";
      for (size_t n = 0; n < N; ++n)
        std::cout << " "
                  << mg.groups[gi].position[ri][static_cast<Eigen::Index>(n)];
      std::cout << " ]\n";
    });
  }
}

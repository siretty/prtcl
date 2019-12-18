#include <catch.hpp>

#include <iostream>
#include <random>
#include <tuple>
#include <vector>

#include <prtcl/data/host/grouped_uniform_grid.hpp>
#include <prtcl/data/integral_grid.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/openmp_neighbourhood.hpp>

#include <boost/range/algorithm/sort.hpp>

TEST_CASE("prtcl/openmp_neighbourhood", "[prtcl][openmp_neighbourhood]") {
  using namespace ::prtcl::expr_literals;

  ::prtcl::data::scheme<float, 3> scheme;

  {
    ::prtcl::integral_grid<3> x_init;
    x_init.extents.fill(16);

    auto &lhs = scheme.add_group("lhs").set_type("lhs");
    lhs.resize(x_init.size());
    auto lhs_x = lhs.add("position"_vvf);

    size_t index = 0;
    for (auto x : x_init)
      lhs_x[index++] = lhs_x.from_range(x);

    auto &rhs = scheme.add_group("rhs").set_type("rhs");
    rhs.resize(x_init.size());
    auto rhs_x = rhs.add("position"_vvf);

    std::mt19937 gen(0);
    std::uniform_real_distribution<float> dis{-.1f, 1.f};

    index = 0;
    for (auto x : x_init) {
      rhs_x[index] = rhs_x.from_range(x).array();
      rhs_x[index][0] += dis(gen);
      rhs_x[index][1] += dis(gen);
      rhs_x[index][2] += dis(gen);
      ++index;
    }
  }

  auto find_neighbours_prtcl = [](auto &scheme_, auto radius_) {
    ::prtcl::openmp_neighbourhood<::prtcl::grouped_uniform_grid<float, 3>>
        neighbourhood;

    neighbourhood.load(scheme_);
    neighbourhood.set_radius(radius_);
    neighbourhood.update();

    std::vector<std::vector<std::vector<std::vector<size_t>>>> neighbours;
    neighbours.resize(scheme_.get_group_count());

    size_t count = 0;

    for (size_t gi = 0; gi < scheme_.get_group_count(); ++gi) {
      auto &g = scheme_.get_group(gi);
      neighbours[gi].resize(g.size());
      for (size_t pi = 0; pi < g.size(); ++pi) {
        neighbours[gi][pi].resize(scheme_.get_group_count());
        for (size_t ngi = 0; ngi < neighbours[gi][pi].size(); ++ngi)
          neighbours[gi][pi][ngi].clear();
        neighbourhood.neighbours(
            gi, pi, [&ns = neighbours[gi][pi], &count](auto ngi, auto npi) {
              ns[ngi].push_back(npi);
              ++count;
            });
        for (size_t ngi = 0; ngi < neighbours[gi][pi].size(); ++ngi)
          boost::range::sort(neighbours[gi][pi][ngi]);
      }
    }

    return std::make_tuple(count, neighbours);
  };

  auto find_neighbours_naive = [](auto &scheme_, auto radius) {
    using namespace ::prtcl::expr_literals;
    std::vector<std::vector<std::vector<std::vector<size_t>>>> neighbours;
    neighbours.resize(scheme_.get_group_count());

    size_t count = 0;

    for (size_t gi = 0; gi < scheme_.get_group_count(); ++gi) {
      auto &g = scheme_.get_group(gi);
      auto g_x = g.get("position"_vvf);
      neighbours[gi].resize(g.size());

      for (size_t pi = 0; pi < g.size(); ++pi) {
        neighbours[gi][pi].resize(scheme_.get_group_count());

        for (size_t ngi = 0; ngi < neighbours[gi][pi].size(); ++ngi) {
          auto &ng = scheme_.get_group(ngi);
          auto ng_x = ng.get("position"_vvf);
          neighbours[gi][pi][ngi].clear();

          for (size_t npi = 0; npi < ng.size(); ++npi) {
            if ((g_x[pi] - ng_x[npi]).norm() < radius) {
              neighbours[gi][pi][ngi].push_back(npi);
              ++count;
            }
          }
        }
      }
    }

    return std::make_tuple(count, neighbours);
  };

  auto [prtcl_count, prtcl_neighbours] = find_neighbours_prtcl(scheme, 2);
  auto [naive_count, naive_neighbours] = find_neighbours_naive(scheme, 2);

  REQUIRE(0 != prtcl_count);
  REQUIRE(prtcl_count == naive_count);

  REQUIRE(boost::range::equal(
      prtcl_neighbours, naive_neighbours, [](auto &lps_, auto &rps_) -> bool {
        return boost::range::equal(lps_, rps_,
                                   [](auto &lns_, auto &rns_) -> bool {
                                     return boost::range::equal(lns_, rns_);
                                   });
      }));

  std::cerr << scheme.get_group(0).size() << " " << prtcl_count << '\n';
}

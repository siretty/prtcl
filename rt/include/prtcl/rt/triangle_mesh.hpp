#pragma once

#include <prtcl/rt/common.hpp>

#include <array>
#include <vector>
#include <istream>
#include <regex>

#include <cstddef>

#include <boost/range/iterator_range.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_> class triangle_mesh {
public:
  using model_policy = ModelPolicy_;

private:
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;

  static constexpr size_t N = ModelPolicy_::dimensionality;
  static_assert(3 == N, "");

  using rvec_type =
      typename math_policy::template nd_dtype_t<nd_dtype::real, N>;
  using face_type = std::array<size_t, 3>;

public:
  auto vertex_postion() const {
    return boost::make_iterator_range(_vertex_postion);
  }

  auto face_vertices() const {
    return boost::make_iterator_range(_face_vertices);
  }

private:
  std::vector<rvec_type> _vertex_postion;
  std::vector<face_type> _face_vertices;

public:
  static auto load_from_obj(std::istream &i_) {
    triangle_mesh mesh;

    static std::regex const re_v{R"(^v\s$)"};

    return mesh;
  }
};

} // namespace prtcl::rt

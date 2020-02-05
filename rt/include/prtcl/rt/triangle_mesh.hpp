#pragma once

#include <prtcl/rt/common.hpp>

#include <array>
#include <iostream>
#include <istream>
#include <regex>
#include <string>
#include <vector>

#include <cstddef>

#include <boost/lexical_cast.hpp>

#include <boost/range/iterator_range.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_> class triangle_mesh {
public:
  using model_policy = ModelPolicy_;

  using index_type = unsigned int;

private:
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;

  using real = typename type_policy::real;

  using rvec_type =
      typename math_policy::template nd_dtype_t<nd_dtype::real, 3>;
  using face_type = std::array<index_type, 3>;

public:
  auto vertices() const { return boost::make_iterator_range(_vertex_postion); }

  auto faces() const { return boost::make_iterator_range(_face_vertices); }

private:
  std::vector<rvec_type> _vertex_postion;
  std::vector<face_type> _face_vertices;

public:
  static auto load_from_obj(std::istream &i_) {
    triangle_mesh mesh;

    static std::string const f = R"([-]?\d+[.]?\d*)";
    static std::string const i = R"(\d+)";

    static auto const re_comment = std::regex{"^(?:#.*|\\s*)?$"};
    static auto const re_vertex =
        std::regex{"^v\\s+(" + f + ")\\s+(" + f + ")\\s+(" + f + ")(?:\\s+(" +
                   f + "))?\\s*$"};
    static auto const re_face =
        std::regex{"^f\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*$"};

    for (std::string line; std::getline(i_, line);) {
      std::smatch m;
      if (std::regex_match(line, re_comment)) {
        continue;
      } else if (std::regex_match(line, m, re_vertex)) {
        mesh._vertex_postion.emplace_back(
            boost::lexical_cast<real>(m[1]), boost::lexical_cast<real>(m[2]),
            boost::lexical_cast<real>(m[3]));
      } else if (std::regex_match(line, m, re_face)) {
        mesh._face_vertices.push_back(
            {boost::lexical_cast<index_type>(m[1]) - 1,
             boost::lexical_cast<index_type>(m[2]) - 1,
             boost::lexical_cast<index_type>(m[3]) - 1});
      } else {
        std::cerr << "ignoring invalid line in .obj file" << '\n';
      }
    }

    return mesh;
  }
};

} // namespace prtcl::rt

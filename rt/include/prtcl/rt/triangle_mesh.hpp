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
  auto &scale(real factor_) {
    for (auto &v : _vertices)
      v *= factor_;
    return *this;
  }

  auto &scale(rvec_type factors_) {
    for (auto &v : _vertices)
      v.array() *= factors_.array();
    return *this;
  }

  auto &translate(rvec_type translation_) {
    for (auto &v : _vertices)
      v += translation_;
    return *this;
  }

public:
  auto vertices() const { return boost::make_iterator_range(_vertices); }

  auto faces() const { return boost::make_iterator_range(_faces); }

private:
  std::vector<rvec_type> _vertices;
  std::vector<face_type> _faces;

public:
  static auto load_from_obj(std::istream &i_) {
    triangle_mesh mesh;

    // NOTE: .*? is a non-greedy match

    // regex to match numbers in decimal point notation
    // static std::string const f = R"((-?\d*(?:[.]\d*)?))";
    static std::string const f =
        R"(([-+]?[0-9]*[.]?[0-9]+(?:[eE][-+]?[0-9]+)?))";

    // matches comments
    static std::regex const re_comment{R"(^(?:#.*|\s*)?$)"};

    // matches groups, captures the name of the group in the first match group
    static std::regex const re_group{R"(^g\s+(.*?)\s*$)"};

    // matches vertices (x y z)
    static std::regex const re_vertex_generic{R"(^v\s+.*$)"};
    static std::regex const re_vertex3{R"(^v\s+)" + f + R"(\s+)" + f +
                                       R"(\s+)" + f + R"(\s*$)"};

    // matches faces (v/t/n v/t/n v/t/n ...)
    static std::regex const re_face_generic{
        R"(^f(?:\s+(?:\d+)(?:[/]\d*(?:[/]\d*)?)?)*\s*$)"};
    static std::regex const re_face3{
        R"(^f\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s*$)"};
    static std::regex const re_face4{
        R"(^f\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s+(\d+)(?:[/]\d*(?:[/]\d*)?)?\s*$)"};

    size_t vertex_count = 0, face_count = 0;

    for (std::string line; std::getline(i_, line);) {
      // remove potential artifacts from line endings of other OSs
      while (line.size() > 0 and
             ('\r' == line[line.size() - 1] or '\n' == line[line.size() - 1]))
        line.resize(line.size() - 1);

      // ignore empty lines and comments
      if (0 == line.size() or std::regex_match(line, re_comment))
        continue;

      std::smatch m;

      // match vertices
      // TODO: maybe include vt and vn
      if (std::regex_match(line, m, re_vertex_generic)) {
        if (std::regex_match(line, m, re_vertex3)) {
          ++vertex_count;
          real const e0 = boost::lexical_cast<real>(m[1]),
                     e1 = boost::lexical_cast<real>(m[2]),
                     e2 = boost::lexical_cast<real>(m[3]);
          mesh._vertices.emplace_back(e0, e1, e2);
        }
        continue;
      }

      // match faces
      if (std::regex_match(line, m, re_face_generic)) {
        if (std::regex_match(line, m, re_face3)) {
          ++face_count;
          auto const i0 = boost::lexical_cast<index_type>(m[1]) - 1,
                     i1 = boost::lexical_cast<index_type>(m[2]) - 1,
                     i2 = boost::lexical_cast<index_type>(m[3]) - 1;
          mesh._faces.push_back({i0, i1, i2});
          continue;
        }

        if (std::regex_match(line, m, re_face4)) {
          ++face_count;
          auto const i0 = boost::lexical_cast<index_type>(m[1]) - 1,
                     i1 = boost::lexical_cast<index_type>(m[2]) - 1,
                     i2 = boost::lexical_cast<index_type>(m[3]) - 1,
                     i3 = boost::lexical_cast<index_type>(m[4]) - 1;
          mesh._faces.push_back({i0, i1, i2});
          mesh._faces.push_back({i0, i2, i3});
          continue;
        }
      }

      // std::cerr << "ignoring invalid line in .obj file" << '\n';
    }

    std::cerr << "vertex count " << vertex_count << '\n';
    std::cerr << "face count " << face_count << '\n';

    return mesh;
  }
};

} // namespace prtcl::rt

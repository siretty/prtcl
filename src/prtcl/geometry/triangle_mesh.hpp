#pragma once

#include "../log.hpp"
#include "../math.hpp"

#include <array>
#include <iostream>
#include <istream>
#include <regex>
#include <string>
#include <vector>

#include <cstddef>

#include <boost/lexical_cast.hpp>

#include <boost/range/iterator_range.hpp>

namespace prtcl {

class TriangleMesh {
public:
  using index_type = unsigned int;

private:
  using Real = double;
  using RVec3 = TensorT<Real, 3>;

  using Face = std::array<index_type, 3>;

public:
  auto &Scale(Real factor) {
    for (auto &v : vertices_)
      v *= factor;
    return *this;
  }

  auto &Scale(RVec3 factors) {
    for (auto &v : vertices_)
      v = math::cmul(v, factors);
    return *this;
  }

  auto &Translate(RVec3 translation_) {
    for (auto &v : vertices_)
      v += translation_;
    return *this;
  }

  auto &Rotate(Real angle, RVec3 axis) {
    auto R = math::RotationMatrixFromAngleAxis(angle, math::normalized(axis));
    for (auto &v : vertices_)
      v = R * v;
    return *this;
  }

public:
  auto Vertices() const { return boost::make_iterator_range(vertices_); }

  auto Faces() const { return boost::make_iterator_range(faces_); }

private:
  std::vector<RVec3> vertices_;
  std::vector<Face> faces_;

public:
  static auto load_from_obj(std::istream &i_) {
    TriangleMesh mesh;

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
    static std::regex const re_vertex3{
        R"(^v\s+)" + f + R"(\s+)" + f + R"(\s+)" + f + R"(\s*$)"};

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
          Real const e0 = boost::lexical_cast<Real>(m[1]),
                     e1 = boost::lexical_cast<Real>(m[2]),
                     e2 = boost::lexical_cast<Real>(m[3]);
          mesh.vertices_.emplace_back(e0, e1, e2);
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
          mesh.faces_.push_back({i0, i1, i2});
          continue;
        }

        if (std::regex_match(line, m, re_face4)) {
          ++face_count;
          auto const i0 = boost::lexical_cast<index_type>(m[1]) - 1,
                     i1 = boost::lexical_cast<index_type>(m[2]) - 1,
                     i2 = boost::lexical_cast<index_type>(m[3]) - 1,
                     i3 = boost::lexical_cast<index_type>(m[4]) - 1;
          mesh.faces_.push_back({i0, i1, i2});
          mesh.faces_.push_back({i0, i2, i3});
          continue;
        }
      }

      // log::Debug("lib", "TriangleMesh", "ignoring invalid line in .obj
      // file");
    }

    log::Debug(
        "lib", "TriangleMesh", "loaded .obj file with ", vertex_count,
        " vertices and ", face_count, " faces");

    return mesh;
  }
};

} // namespace prtcl

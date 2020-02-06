#pragma once

#include <prtcl/rt/triangle_mesh.hpp>

#include <variant>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_> class triangle_model {
public:
  using model_policy = ModelPolicy_;
  using triangle_mesh_type = triangle_mesh<model_policy>;

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
    for (auto &m : _meshes)
      m.scale(factor_);
    return *this;
  }

  auto &translate(rvec_type translation_) {
    for (auto &m : _meshes)
      m.translate(translation_);
    return *this;
  }

public:
  auto meshes() const { return boost::make_iterator_range(_meshes); }

private:
  std::vector<triangle_mesh_type> _meshes;

public:
  static auto load_from_obj(std::istream &i_) {
    triangle_model model;
    auto &mesh = model._meshes.emplace_back();

    // NOTE: .*? is a non-greedy match

    // regex to match numbers in decimal point notation
    static std::string const f = R"((-?\d*(?:[.]\d*)))";

    // matches comments
    static std::regex const re_comment{R"(^(?:#.*|\s*)?$)"};

    // matches groups, captures the name of the group in the first match group
    static std::regex const re_group{R"(^g\s+(.*?)\s*$)"};

    // matches vertices (x y z)
    static std::regex const re_vertex3{R"(^v\s+)" + f + R"(\s+)" + f +
                                       R"(\s+)" + f + R"(\s*$)"};

    // matches faces (v/t/n v/t/n v/t/n ...)
    static std::regex const re_face_generic{
        R"(^f((?:\s+\d+(?:[/]\d*(?:[/]\d*)))+)\s*$)"};
    static std::regex const re_face_part{R"(\d+([/]\d*([/]\d*)))"};

    // read stream line by line
    for (std::string line; std::getline(i_, line);) {
      std::smatch m;
      if (std::regex_match(line, re_comment)) {
        // ignore comments
        continue;
      } else if (std::regex_match(line, m, re_vertex3)) {
        // extract vertex coordinates
        //mesh._vertices.emplace_back(
        //    boost::lexical_cast<real>(m[1]), boost::lexical_cast<real>(m[2]),
        //    boost::lexical_cast<real>(m[3]));
      } else if (std::regex_match(line, m, re_face_generic)) {
        std::cerr << " [ " << m[1] << " ]" << '\n';
        // mesh._faces.push_back({boost::lexical_cast<index_type>(m[1]) - 1,
        //                       boost::lexical_cast<index_type>(m[2]) - 1,
        //                       boost::lexical_cast<index_type>(m[3]) - 1});
      } else {
        //std::cerr << "ignoring invalid line in .obj file" << '\n';
      }
    }

    return model;
  }
};

} // namespace prtcl::rt

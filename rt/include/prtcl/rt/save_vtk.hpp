#pragma once

#include <prtcl/rt/basic_model.hpp>

#include <ostream>

namespace prtcl::rt {

template <typename ModelPolicy_>
void save_vtk(std::ostream &o_, basic_group<ModelPolicy_> const &g_) {
  constexpr size_t N = ModelPolicy_::dimensionality;

  o_ << "# vtk DataFile Version 2.0\n";
  o_ << g_.get_name() << " " << g_.get_type() << "\n";
  o_ << "ASCII\n";
  o_ << "DATASET POLYDATA\n";

  auto o_flags = o_.flags();

  auto format_array = [](auto const &value) {
    std::ostringstream s;
    using size_type = decltype(value.size());
    s << value[0];
    for (size_type i = 1; i < value.size(); ++i)
      s << " " << value[i];
    return s.str();
  };

  auto x = g_.template get_varying<nd_dtype::real, N>("position");
  o_ << "POINTS " << g_.size() << " float\n";
  for (size_t i = 0; i < g_.size(); ++i)
    o_ << std::fixed << format_array(x[i]) << "\n";

  o_ << "POINT_DATA " << g_.size() << "\n";

  if (g_.template has_varying<nd_dtype::real, N>("velocity")) {
    auto v = g_.template get_varying<nd_dtype::real, N>("velocity");
    o_ << "VECTORS velocity float\n";
    for (size_t i = 0; i < g_.size(); ++i)
      o_ << std::fixed << format_array(v[i]) << "\n";
  }

  if (g_.template has_varying<nd_dtype::real, N>("acceleration")) {
    auto a = g_.template get_varying<nd_dtype::real, N>("acceleration");
    o_ << "VECTORS acceleration float\n";
    for (size_t i = 0; i < g_.size(); ++i)
      o_ << std::fixed << format_array(a[i]) << "\n";
  }

  if (g_.template has_varying<nd_dtype::real>("volume")) {
    auto v = g_.template get_varying<nd_dtype::real>("volume");
    o_ << "SCALARS volume float 1\n";
    o_ << "LOOKUP_TABLE default\n";
    for (size_t i = 0; i < g_.size(); ++i)
      o_ << std::fixed << v[i] << "\n";
  }

  if (g_.template has_varying<nd_dtype::real>("density")) {
    auto d = g_.template get_varying<nd_dtype::real>("density");
    o_ << "SCALARS density float 1\n";
    o_ << "LOOKUP_TABLE default\n";
    for (size_t i = 0; i < g_.size(); ++i)
      o_ << std::fixed << d[i] << "\n";
  }

  if (g_.template has_varying<nd_dtype::real>("pressure")) {
    auto p = g_.template get_varying<nd_dtype::real>("pressure");
    o_ << "SCALARS pressure float 1\n";
    o_ << "LOOKUP_TABLE default\n";
    for (size_t i = 0; i < g_.size(); ++i)
      o_ << std::fixed << p[i] << "\n";
  }

  o_.flags(o_flags);
}

} // namespace prtcl::rt

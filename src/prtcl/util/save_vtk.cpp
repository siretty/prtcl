#include "save_vtk.hpp"

#include "../errors/not_implemented_error.hpp"

#include <ostream>

namespace prtcl {

namespace {

template <typename T>
void WriteVFS(std::ostream &o, VaryingFieldSpan<T> const &v) {
  for (size_t item_i = 0; item_i < v.GetSize(); ++item_i) {
    o << std::fixed << v[item_i] << '\n';
  }
}

template <typename T, size_t N>
void WriteVFS(std::ostream &o, VaryingFieldSpan<T, N> const &v) {
  auto const comp_n = static_cast<math::Index>(N);

  for (size_t item_i = 0; item_i < v.GetSize(); ++item_i) {
    auto item = v[item_i];
    for (math::Index comp_i = 0; comp_i < comp_n; ++comp_i) {
      if (comp_i > 0)
        o << ' ';
      o << std::fixed << item[comp_i];
    }
    o << '\n';
  }
}

void WriteField(
    std::ostream &o, VaryingManager const &varying, std::string_view name) {
  if (auto field = varying.FieldSpan<float>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<float, 1>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<float, 2>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<float, 3>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<double>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<double, 1>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<double, 2>(name))
    return WriteVFS(o, field);
  if (auto field = varying.FieldSpan<double, 3>(name))
    return WriteVFS(o, field);

  throw NotImplementedError{};
}

} // namespace

void SaveVTK(std::ostream &o, Group const &group) {
  o << "# vtk DataFile Version 2.0\n";
  o << group.GetGroupName() << " " << group.GetGroupType() << "\n";
  o << "ASCII\n";
  o << "DATASET POLYDATA\n";

  auto o_flags = o.flags();

  auto const &varying = group.GetVarying();

  o << "POINTS " << group.GetItemCount() << " float\n";
  WriteField(o, varying, "position");

  o << "POINT_DATA " << group.GetItemCount() << "\n";

  if (varying.HasField("velocity")) {
    o << "VECTORS velocity float\n";
    WriteField(o, varying, "velocity");
  }

  if (varying.HasField("acceleration")) {
    o << "VECTORS acceleration float\n";
    WriteField(o, varying, "acceleration");
  }

  o << "SCALARS _index float 1\n";
  o << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.GetItemCount(); ++i)
    o << std::fixed << i << "\n";

  if (varying.HasField("mass")) {
    o << "SCALARS mass float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "mass");
  }

  if (varying.HasField("volume")) {
    o << "SCALARS volume float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "volume");
  }

  if (varying.HasField("density")) {
    o << "SCALARS density float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "density");
  }

  if (varying.HasField("pressure")) {
    o << "SCALARS pressure float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "pressure");
  }

  if (varying.HasField("time_of_birth")) {
    o << "SCALARS time_of_birth float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "time_of_birth");
  }

  if (varying.HasField("implicit_function")) {
    o << "SCALARS implicit_function float 1\n";
    o << "LOOKUP_TABLE default\n";
    WriteField(o, varying, "implicit_function");
  }

  if (varying.HasField("implicit_function_gradient")) {
    o << "VECTORS implicit_function_gradient float\n";
    WriteField(o, varying, "implicit_function_gradient");
  }

  o.flags(o_flags);
}

} // namespace prtcl

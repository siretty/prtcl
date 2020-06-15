#include "save_vtk.hpp"

#include "../data/vector_of_tensors.hpp"
#include "../errors/not_implemented_error.hpp"

#include <ostream>

namespace prtcl {

namespace {

template <typename T>
void WriteVOT(std::ostream &o, VectorOfTensors<T> const &v) {
  auto a = v.GetAccessImpl();

  for (size_t item_i = 0; item_i < a.GetSize(); ++item_i) {
    o << std::fixed << a.GetItem(item_i) << '\n';
  }
}

template <typename T, size_t N>
void WriteVOT(std::ostream &o, VectorOfTensors<T, N> const &v) {
  auto a = v.GetAccessImpl();
  auto const comp_n = static_cast<math::Index>(N);

  for (size_t item_i = 0; item_i < a.GetSize(); ++item_i) {
    auto item = a.GetItem(item_i);
    for (math::Index comp_i = 0; comp_i < comp_n; ++comp_i) {
      if (comp_i > 1)
        o << ' ';
      o << std::fixed << item[comp_i];
    }
    o << '\n';
  }
}

void WriteField(
    std::ostream &o, VaryingManager const &varying, std::string_view name) {
  if (auto *field = varying.TryGetFieldImpl<float>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<float, 1>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<float, 2>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<float, 3>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<double>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<double, 1>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<double, 2>(name))
    return WriteVOT(o, *field);
  if (auto *field = varying.TryGetFieldImpl<double, 3>(name))
    return WriteVOT(o, *field);

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

  o.flags(o_flags);
}

} // namespace prtcl

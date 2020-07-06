#include "module_data.hpp"

#include <prtcl/data/component_type.hpp>
#include <prtcl/data/group.hpp>
#include <prtcl/data/model.hpp>
#include <prtcl/data/shape.hpp>
#include <prtcl/data/tensor_type.hpp>
#include <prtcl/data/uniform_field.hpp>
#include <prtcl/data/uniform_manager.hpp>
#include <prtcl/data/varying_field.hpp>
#include <prtcl/data/varying_manager.hpp>

#include <prtcl/util/save_vtk.hpp>

#include <fstream>

namespace prtcl::lua {

sol::table ModuleData(sol::state_view lua) {
  auto m = lua.create_table();

  {
    auto t = m.new_usertype<ComponentType>(
        "ctype",
        sol::constructors<ComponentType(), ComponentType(std::string_view)>());
    t["valid"] = sol::property(&ComponentType::IsValid);

    t["b"] = sol::property([] { return ComponentType::kBoolean; });
    t["s32"] = sol::property([] { return ComponentType::kSInt32; });
    t["s64"] = sol::property([] { return ComponentType::kSInt64; });
    t["f32"] = sol::property([] { return ComponentType::kFloat32; });
    t["f64"] = sol::property([] { return ComponentType::kFloat64; });
  }

  {
    auto t = m.new_usertype<Shape>(
        "shape", "new",
        sol::factories(
            []() { return Shape{}; },
            [](std::vector<size_t> extents) { return Shape{extents}; }));
    t["rank"] = sol::property(&Shape::GetRank);
    t["extents"] = sol::property(&Shape::GetExtents);
    t["empty"] = sol::property(&Shape::IsEmpty);
  }

  {
    auto t = m.new_usertype<TensorType>(
        "ttype", "new",
        sol::factories(
            []() { return TensorType{}; },
            [](ComponentType ctype, Shape shape) {
              return TensorType{ctype, shape};
            },
            [](std::string_view ctype_str, std::vector<size_t> extents) {
              return TensorType{ComponentType{ctype_str}, Shape{extents}};
            }));
    t["ctype"] = sol::property(&TensorType::GetComponentType);
    t["shape"] = sol::property(&TensorType::GetShape);
    t["component_count"] = sol::property(&TensorType::GetComponentCount);
    t["valid"] = sol::property(&TensorType::IsValid);
    t["empty"] = sol::property(&TensorType::IsEmpty);
    t["with_ctype"] = &TensorType::WithComponentType;
    t["with_ctype_of"] = &TensorType::WithComponentTypeOf;
    t["with_shape"] = &TensorType::WithShape;
    t["with_shape_of"] = &TensorType::WithShapeOf;
  }

  {
    auto t = m.new_usertype<Model>("model", sol::constructors<Model()>());
    t["add_group"] = &Model::AddGroup;
    t["get_group"] = &Model::TryGetGroup;
    t["remove_group"] = &Model::RemoveGroup;
    t["group_count"] = sol::property(&Model::GetGroupCount);

    t["dirty"] = sol::property(&Model::IsDirty, &Model::SetDirty);

    t["global"] = sol::property(&Model::GetGlobal);
    t["add_global_field"] = &Model::AddGlobalField;
    t["remove_global_field"] = &Model::RemoveGlobalField;

    t["group_names"] = &Model::GetGroupNames;

    t.set_function(
        "save_native_binary", [](Model const &self, std::string path) {
          std::fstream file{path, file.out};
          NativeBinaryArchiveWriter archive{file};
          self.Save(archive);
        });

    t.set_function("load_native_binary", [](std::string path) {
      std::fstream file{path, file.in};
      NativeBinaryArchiveReader archive{file};
      Model *model = new Model;
      model->Load(archive);
      return model;
    });
  }

  {
    auto t = m.new_usertype<Group>("group", sol::no_constructor);
    t["group_name"] = sol::property(&Group::GetGroupName);
    t["group_type"] = sol::property(&Group::GetGroupType);
    t["group_index"] = sol::property(&Group::GetGroupIndex);
    t["item_count"] = sol::property(&Group::GetItemCount);

    t["tags"] = sol::property(&Group::GetTags);
    t["add_tag"] = &Group::AddTag;
    t["has_tag"] = &Group::HasTag;
    t["remove_tag"] = &Group::RemoveTag;

    t["dirty"] = sol::property(&Group::IsDirty, &Group::SetDirty);

    t["uniform"] = sol::property(&Group::GetUniform);
    t["add_uniform_field"] = &Group::AddUniformField;

    t["varying"] = sol::property(&Group::GetVarying);
    t["add_varying_field"] = &Group::AddVaryingField;

    t["remove_field"] = &Group::RemoveField;

    t["create_items"] = &Group::CreateItems;
    // TODO: t["destroy_items"] = &Group::DestroyItems;
    t["resize"] = &Group::Resize;
    // TODO: t["permute"] = &Group::Permute;

    t["save_vtk"] = [](Group const &self, std::string path) {
      std::fstream file{path, file.out};
      SaveVTK(file, self);
    };

    t.set_function("translate", [](Group const &self, RealVector const &delta) {
      if (auto x = self.GetVarying().FieldWrap<double, 3>("position")) {
        for (size_t i = 0; i < x.size(); ++i) {
          x.Set(i, x.Get(i) + delta);
        }
      }
    });

    t.set_function("scale", [](Group const &self, RealVector const &factors) {
      if (auto x = self.GetVarying().FieldWrap<double, 3>("position")) {
        for (size_t i = 0; i < x.size(); ++i) {
          x.Set(i, math::cmul(factors, x.Get(i)));
        }
      }
    });

    t.set_function(
        "rotate",
        [](Group const &self, RealScalar angle, RealVector const &axis) {
          auto R = math::RotationMatrixFromAngleAxis(angle, axis);
          if (auto x = self.GetVarying().FieldWrap<double, 3>("position")) {
            for (size_t i = 0; i < x.size(); ++i) {
              x.Set(i, R * x.Get(i));
            }
          }
        });
  }

  {
    auto t =
        m.new_usertype<UniformManager>("uniform_manager", sol::no_constructor);
    t["field_count"] = sol::property(&UniformManager::GetFieldCount);
    t["get_field"] = &UniformManager::GetField;
    t["has_field"] = &UniformManager::HasField;
    t["field_names"] = &UniformManager::GetFieldNames;
  }

  {
    auto t =
        m.new_usertype<VaryingManager>("varying_manager", sol::no_constructor);
    t["field_count"] = sol::property(&VaryingManager::GetFieldCount);
    t["item_count"] = sol::property(&VaryingManager::GetItemCount);
    t["get_field"] = &VaryingManager::GetField;
    t["has_field"] = &VaryingManager::HasField;
    t["field_names"] = &VaryingManager::GetFieldNames;
  }

  {
    auto t = m.new_usertype<UniformField>("uniform_field", sol::no_constructor);
    t.set_function("get", [](UniformField const &self) -> ValueVariant {
      auto const ttype = self.GetType();
      auto const ctype = ttype.GetComponentType();
      if (RepresentsReal(ctype)) {
        switch (ttype.GetShape().GetRank()) {
        case 0: {
          RealScalar result;
          self.Get(result);
          return result;
        } break;
        case 1: {
          RealVector result;
          self.Get(result);
          return result;
        } break;
        case 2: {
          RealMatrix result;
          self.Get(result);
          return result;
        } break;
        default:
          throw NotImplementedError{};
        };
      } else if (RepresentsInteger(ctype)) {
        switch (ttype.GetShape().GetRank()) {
        case 0: {
          IntegerScalar result;
          self.Get(result);
          return result;
        } break;
        case 1: {
          IntegerVector result;
          self.Get(result);
          return result;
        } break;
        case 2: {
          IntegerMatrix result;
          self.Get(result);
          return result;
        } break;
        default:
          throw NotImplementedError{};
        };
      } else
        throw NotImplementedError{};
    });
    t.set_function(
        "set", sol::overload(
                   [](UniformField &self, RealScalar const &value) {
                     self.Set(value);
                   },
                   [](UniformField &self, RealVector const &value) {
                     self.Set(value);
                   },
                   [](UniformField &self, RealMatrix const &value) {
                     self.Set(value);
                   },
                   [](UniformField &self, IntegerScalar const &value) {
                     self.Set(value);
                   },
                   [](UniformField &self, IntegerVector const &value) {
                     self.Set(value);
                   },
                   [](UniformField &self, IntegerMatrix const &value) {
                     self.Set(value);
                   }));
  }

  {
    auto t = m.new_usertype<VaryingField>("varying_field", sol::no_constructor);
    t.set_function(
        "get", [](VaryingField const &self, size_t index) -> ValueVariant {
          auto const ttype = self.GetType();
          auto const ctype = ttype.GetComponentType();
          if (RepresentsReal(ctype)) {
            switch (ttype.GetShape().GetRank()) {
            case 0: {
              RealScalar result;
              self.Get(index, result);
              return result;
            } break;
            case 1: {
              RealVector result;
              self.Get(index, result);
              return result;
            } break;
            case 2: {
              RealMatrix result;
              self.Get(index, result);
              return result;
            } break;
            default:
              throw NotImplementedError{};
            };
          } else
            throw NotImplementedError{};
        });
    t.set_function(
        "set", sol::overload(
                   [](VaryingField &self, size_t index,
                      RealScalar const &value) { self.Set(index, value); },
                   [](VaryingField &self, size_t index,
                      RealVector const &value) { self.Set(index, value); },
                   [](VaryingField &self, size_t index,
                      RealMatrix const &value) { self.Set(index, value); }));
  }

  return m;
}

} // namespace prtcl::lua

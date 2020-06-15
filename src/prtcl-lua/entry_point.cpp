//
// Created by daned on 6/12/20.
//

#include "entry_point.hpp"

#include <prtcl/data/group.hpp>
#include <prtcl/data/model.hpp>
#include <prtcl/data/uniform_manager.hpp>
#include <prtcl/data/varying_manager.hpp>

#include <prtcl/util/scheduler.hpp>
#include <prtcl/util/virtual_clock.hpp>
#include <prtcl/util/save_vtk.hpp>

#include <prtcl/errors/invalid_shape_error.hpp>

#include <prtcl/log.hpp>

#include <vector>
#include <fstream>

#include <sol/sol.hpp>

namespace prtcl::lua {
namespace {

template <typename Lua>
static auto ComponentVariantToLua(Lua &lua, ComponentVariant const &variant) {
  return std::visit(
      [&lua](auto value) {
        if constexpr (std::is_same_v<decltype(value), float>)
          return sol::make_object(lua, static_cast<double>(value));
        else
          return sol::make_object(lua, value);
      },
      variant);
};

static ComponentVariant
LuaToComponentVariant(ComponentType const &ctype, sol::object value) {
  if (value.is<bool>() and ctype == ComponentType::kBoolean)
    return value.as<bool>();
  else if (ctype == ComponentType::kSInt32)
    return value.as<int32_t>();
  else if (ctype == ComponentType::kSInt64)
    return value.as<int64_t>();
  else if (ctype == ComponentType::kFloat32)
    return value.as<float>();
  else if (ctype == ComponentType::kFloat64)
    return value.as<double>();
  else
    throw NotImplementedError{};
}

static constexpr void CXXToLuaIndex(size_t &index) {
  assert(index >= 1);
  index -= 1;
}

static constexpr void CXXToLuaIndex(cxx::span<size_t> cidx) {
  for (auto &index : cidx)
    CXXToLuaIndex(index);
}

static auto ModuleLog(sol::state_view lua) {
  auto m = lua.create_table();

  m["debugf"] = [](sol::this_state s, sol::object self, std::string origin,
                   sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Debug("lua", origin, message);
    return self;
  };

  m["infof"] = [](sol::this_state s, sol::object self, std::string origin,
                  sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Info("lua", origin, message);
    return self;
  };

  m["warningf"] = [](sol::this_state s, sol::object self, std::string origin,
                     sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Warning("lua", origin, message);
    return self;
  };

  m["errorf"] = [](sol::this_state s, sol::object self, std::string origin,
                   sol::variadic_args args) {
    sol::state_view lua{s};
    std::string message = lua["string"]["format"](args);
    ::prtcl::log::Error("lua", origin, message);
    return self;
  };

  return m;
}

static auto ModuleData(sol::state_view lua) {
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
  }

  {
    auto t =
        m.new_usertype<UniformManager>("uniform_manager", sol::no_constructor);
    t["field_count"] = sol::property(&UniformManager::GetFieldCount);
    t["get_field"] = &UniformManager::TryGetField;
    t["has_field"] = &UniformManager::HasField;
    t["field_names"] = &UniformManager::GetFieldNames;
  }

  {
    auto t =
        m.new_usertype<VaryingManager>("varying_manager", sol::no_constructor);
    t["field_count"] = sol::property(&VaryingManager::GetFieldCount);
    t["item_count"] = sol::property(&VaryingManager::GetItemCount);
    t["get_field"] = &VaryingManager::TryGetField;
    t["has_field"] = &VaryingManager::HasField;
    t["field_names"] = &VaryingManager::GetFieldNames;
  }

  {
    auto t = m.new_usertype<CollectionOfMutableTensors>(
        "collection_of_mutable_tensors", sol::no_constructor);
    t["type"] = sol::property(&CollectionOfMutableTensors::GetType);
    t["size"] = sol::property(&CollectionOfMutableTensors::GetSize);
    t["get_access"] = &CollectionOfMutableTensors::GetAccess;
  }

  {
    auto t = m.new_usertype<AccessToMutableTensors>(
        "access_to_mutable_tensors", sol::no_constructor);
    t["type"] = sol::property(&AccessToMutableTensors::GetType);
    t["size"] = sol::property(&AccessToMutableTensors::GetSize);

    t["get_component"] = [](AccessToMutableTensors const &self, size_t item,
                            std::vector<size_t> cidx, sol::this_state s) {
      CXXToLuaIndex(item);
      CXXToLuaIndex(cidx);
      sol::state_view lua{s};
      return ComponentVariantToLua(lua, self.GetComponentVariant(item, cidx));
    };

    t["set_component"] = [](AccessToMutableTensors const &self, size_t item,
                            std::vector<size_t> cidx, sol::object value) {
      CXXToLuaIndex(item);
      CXXToLuaIndex(cidx);
      auto const ctype = self.GetType().GetComponentType();
      self.SetComponentVariant(item, cidx, LuaToComponentVariant(ctype, value));
    };

    t["get_item"] = [](AccessToMutableTensors const &self, size_t item,
                       sol::this_state s) -> sol::object {
      CXXToLuaIndex(item);
      sol::state_view lua{s};
      auto const &ttype = self.GetType();
      auto const &shape = ttype.GetShape();
      auto const rank = shape.GetRank();
      if (rank == 0) {
        return ComponentVariantToLua(lua, self.GetComponentVariant(item, {}));
      } else if (rank == 1) {
        auto result = lua.create_table(shape[0]);
        for (size_t row = 0; row < shape[0]; ++row) {
          result[row + 1] =
              ComponentVariantToLua(lua, self.GetComponentVariant(item, {row}));
        }
        return result;
      } else if (rank == 2) {
        auto result = lua.create_table(shape[0]);
        for (size_t row = 0; row < shape[0]; ++row) {
          result[row + 1] = lua.create_table(shape[1]);
          for (size_t col = 0; col < shape[1]; ++col) {
            result[row + 1][col + 1] = ComponentVariantToLua(
                lua, self.GetComponentVariant(item, {row, col}));
          }
        }
        return result;
      } else
        throw NotImplementedError{};
    };

    t["set_item"] = [](AccessToMutableTensors const &self, size_t item,
                       sol::object value) {
      CXXToLuaIndex(item);
      auto const &ttype = self.GetType();
      auto const &ctype = ttype.GetComponentType();
      auto const &shape = ttype.GetShape();
      auto const rank = shape.GetRank();
      if (rank == 0) {
        self.SetComponentVariant(item, {}, LuaToComponentVariant(ctype, value));
      } else if (rank == 1) {
        auto outer = value.as<sol::table>();
        if (outer.size() != shape[0])
          throw InvalidShapeError{};
        for (size_t row = 0; row < shape[0]; ++row) {
          self.SetComponentVariant(
              item, {row}, LuaToComponentVariant(ctype, outer[row + 1]));
        }
      } else if (rank == 2) {
        auto outer = value.as<sol::table>();
        if (outer.size() != shape[0])
          throw InvalidShapeError{};
        for (size_t row = 0; row < shape[0]; ++row) {
          auto inner = outer[row + 1].get<sol::table>();
          if (inner.size() != shape[1])
            throw InvalidShapeError{};
          for (size_t col = 0; col < shape[1]; ++col) {
            self.SetComponentVariant(
                item, {row, col}, LuaToComponentVariant(ctype, inner[col + 1]));
          }
        }
      } else
        throw NotImplementedError{};
    };
  }

  return m;
}

static auto ModuleUtil(sol::state_view lua) {
  auto m = lua.create_table();

  {
    auto t = m.new_usertype<VirtualClock>("virtual_clock", sol::no_constructor);
    t["now"] = [](VirtualClock const &self) {
      return self.now().time_since_epoch().count();
    };

    t["reset"] = &VirtualClock::reset;

    t["set"] = [](VirtualClock &self, double when) {
      self.set(VirtualClock::time_point{VirtualClock::duration{when}});
    };

    t["advance"] = [](VirtualClock &self, double dur) {
      return self.advance(dur).time_since_epoch().count();
    };
  }

  {
    auto t = m.new_usertype<VirtualScheduler>(
        "virtual_scheduler", sol::constructors<VirtualScheduler()>());
    t["get_clock"] = &VirtualScheduler::GetClockPtr;

    t["tick"] = &VirtualScheduler::Tick;
    t["clear"] = &VirtualScheduler::Clear;

    t["do_nothing"] = &VirtualScheduler::DoNothing;
    t["reschedule_after"] = [](VirtualScheduler const &self, double after) {
      return self.RescheduleAfter(after);
    };

    t["schedule_at"] = [](VirtualScheduler &self, double when,
                          sol::protected_function callback) {
      VirtualScheduler::TimePoint when_tp{VirtualScheduler::Duration{when}};
      self.ScheduleAt(
          when_tp,
          [callback](VirtualScheduler &self, auto delay)
              -> VirtualScheduler::CallbackReturnType {
            return callback(self, delay.count());
          });
    };

    t["schedule_after"] = [](VirtualScheduler &self, double after,
                             sol::protected_function callback) {
      VirtualScheduler::Duration after_dur{after};
      self.ScheduleAfter(
          after_dur,
          [callback](VirtualScheduler &self, auto delay)
              -> VirtualScheduler::CallbackReturnType {
            return callback(self, delay.count());
          });
    };
  }

  return m;
}

static auto ModuleEntryPoint(sol::state_view lua) {
  auto m = lua.create_table();
  m["log"] = ModuleLog(lua);
  m["data"] = ModuleData(lua);
  m["util"] = ModuleUtil(lua);
  return m;
}

static int RawModuleEntryPoint(lua_State *L) {
  sol::state_view lua{L};

  // TODO: add error and exception handling
  auto m = ModuleEntryPoint(lua);

  sol::stack::push(lua.lua_state(), m);
  return 1;
}

} // namespace
} // namespace prtcl::lua

extern "C" {

int luaopen_libprtcl_lua(lua_State *L) {
  ::prtcl::log::Debug(
      "app", "prtcl-lua", "luaopen_libprtcl_lua(", static_cast<void *>(L), ")");

  return ::prtcl::lua::RawModuleEntryPoint(L);
}

int luaopen_prtcl(lua_State *L) {
  ::prtcl::log::Debug(
      "app", "prtcl-lua", "luaopen_prtcl(", static_cast<void *>(L), ")");

  return luaopen_libprtcl_lua(L);
}

} // extern "C"
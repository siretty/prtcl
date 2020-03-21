#pragma once

#include <prtcl/core/ndtype.hpp>
#include <prtcl/core/range.hpp>

#include <prtcl/gt/ast.hpp>

#include <tuple>
#include <unordered_map>

namespace prtcl::gt {

struct field_map {
public:
  size_t size() const { return name_to_type.size(); }

  bool empty() const { return size() == 0; }

public:
  bool has_alias(std::string alias) const {
    return alias_to_name.find(alias) != alias_to_name.end();
  }

public:
  auto all() const {
    using namespace core::range_adaptors;
    return alias_to_name | transformed([this](auto an) {
             auto it = name_to_type.find(std::get<1>(an));
             if (it == name_to_type.end())
               throw std::runtime_error{
                   "resolved field alias not found in name map"};
             return std::make_tuple(
                 std::get<0>(an), std::get<1>(an), it->second);
           });
  }

public:
  template <typename Field_> void insert(Field_ const &field) {
    auto &a2n = alias_to_name;
    auto &n2t = name_to_type;

    // TODO: context with error_handler
    // TODO: think about allowing multiple aliases of the same field
    if (auto it = a2n.find(field.alias); it != a2n.end()) {
      throw std::runtime_error{
          "multiple aliases of the same field are not allowed"};
    }

    // TODO: context with error_handler
    if (auto it = n2t.find(field.name); it != n2t.end()) {
      throw std::runtime_error{
          "multiple fields with different types are not allowed"};
    }

    a2n.insert({field.alias, field.name});
    n2t.insert({field.name, field.type});
  }

  auto name_for_alias(std::string alias) const {
    auto &a2n = alias_to_name;

    if (auto it = a2n.find(alias); it != a2n.end()) {
      return it->second;
    } else {
      throw std::runtime_error{"alias not mapped to a field name"};
    }
  }

  auto type_for_alias(std::string alias) const {
    auto &n2t = name_to_type;

    auto name = name_for_alias(alias);

    if (auto it = n2t.find(name); it != n2t.end()) {
      return it->second;
    } else {
      throw std::runtime_error{"field name not mapped to field type"};
    }
  }

  std::unordered_map<std::string, std::string> alias_to_name;
  std::unordered_map<std::string, core::ndtype> name_to_type;
};

} // namespace prtcl::gt

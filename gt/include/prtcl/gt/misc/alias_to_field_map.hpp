#pragma once

#include "../ast.hpp"

#include "field_less.hpp"

#include <optional>
#include <set>
#include <unordered_map>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class alias_to_field_map {
public:
  bool contains(std::string const &alias_name_) const {
    return _map.find(alias_name_) != _map.end();
  }

  std::optional<ast::init::field> search(std::string const &key_) const {
    if (auto it = _map.find(key_); it != _map.end())
      return it->second;
    else
      return std::nullopt;
  }

  auto items() const { return boost::make_iterator_range(_map); }

  auto const &values() const { return _field_set; }

public:
  auto resolver() const {
    return boost::adaptors::transformed(
        [this](auto const &alias) { return this->search(alias).value(); });
  }

public:
  friend alias_to_field_map
  make_alias_to_field_map(ast::prtcl_source_file const &);

private:
  std::unordered_map<std::string, ast::init::field> _map;
  std::set<ast::init::field, field_less> _field_set;
};

alias_to_field_map
make_alias_to_field_map(ast::prtcl_source_file const &node_) {
  alias_to_field_map result;

  // TODO: introduce and handle proper scoping
  for (auto const &statement : node_.statements) {
    if (auto *let = std::get_if<ast::stmt::let>(&statement)) {
      if (auto *field = std::get_if<ast::init::field>(&let->initializer)) {
        result._map.insert({let->alias_name, *field});
        result._field_set.insert(*field);
      }
    }
  }

  return result;
}

} // namespace prtcl::gt

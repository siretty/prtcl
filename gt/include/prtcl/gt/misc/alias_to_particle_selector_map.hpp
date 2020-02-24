#pragma once

#include "../ast.hpp"

#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

struct particle_selector_less {
  bool operator()(
      ast::init::particle_selector const &lhs_,
      ast::init::particle_selector const &rhs_) const {
    std::vector<std::string> lhs_types = lhs_.type_disjunction,
                             rhs_types = rhs_.type_disjunction,
                             lhs_tags = lhs_.tag_conjunction,
                             rhs_tags = rhs_.tag_conjunction;
    boost::range::sort(lhs_types);
    boost::range::sort(rhs_types);
    boost::range::sort(lhs_tags);
    boost::range::sort(rhs_tags);
    if (boost::range::equal(lhs_types, rhs_types)) {
      return boost::range::lexicographical_compare(lhs_tags, rhs_tags);
    } else {
      return boost::range::lexicographical_compare(lhs_types, rhs_types);
    }
  }
};

class alias_to_particle_selector_map {
public:
  bool contains(std::string const &alias_name_) const {
    return _map.find(alias_name_) != _map.end();
  }

  std::optional<ast::init::particle_selector>
  search(std::string const &key_) const {
    if (auto it = _map.find(key_); it != _map.end())
      return it->second;
    else
      return std::nullopt;
  }

  auto items() const { return boost::make_iterator_range(_map); }

  auto const &values() const { return _set; }

public:
  friend alias_to_particle_selector_map
  make_alias_to_particle_selector_map(ast::prtcl_source_file const &);

private:
  std::unordered_map<std::string, ast::init::particle_selector> _map;
  std::set<ast::init::particle_selector, particle_selector_less> _set;
};

alias_to_particle_selector_map
make_alias_to_particle_selector_map(ast::prtcl_source_file const &node_) {
  alias_to_particle_selector_map result;

  // TODO: introduce and handle proper scoping
  for (auto const &statement : node_.statements) {
    if (auto *let = std::get_if<ast::stmt::let>(&statement)) {
      if (auto *sel =
              std::get_if<ast::init::particle_selector>(&let->initializer)) {
        result._map.insert({let->alias_name, *sel});
        result._set.insert(*sel);
      }
    }
  }

  return result;
}

} // namespace prtcl::gt

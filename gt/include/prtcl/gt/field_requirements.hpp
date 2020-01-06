#pragma once

#include <boost/range/iterator_range_core.hpp>
#include <prtcl/gt/field.hpp>

#include <set>
#include <string>
#include <utility>

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class field_requirements {
public:
  void add_requirement(field f_) {
    if (field_kind::global != f_.kind())
      throw "invalid field kind";
    _global.insert(f_);
  }

  void add_requirement(std::string t_, field f_) {
    switch (f_.kind()) {
    case field_kind::uniform:
    case field_kind::varying: {
      _fields.left.insert({t_, f_});
    } break;
    default:
      throw "invalid field kind";
    }
  }

public:
  auto global_fields() const { return ::boost::make_iterator_range(_global); }

  auto fields(std::string t_) const {
    return ::boost::make_iterator_range(
               _fields.left.lower_bound(t_), _fields.left.upper_bound(t_)) |
           boost::adaptors::map_values;
  }

  auto uniform_fields(std::string t_) const {
    return fields(t_) | boost::adaptors::filtered([](auto &&f_) {
             return field_kind::uniform ==
                    std::forward<decltype(f_)>(f_).kind();
           });
  }

  auto varying_fields(std::string t_) const {
    return fields(t_) | boost::adaptors::filtered([](auto &&f_) {
             return field_kind::varying ==
                    std::forward<decltype(f_)>(f_).kind();
           });
  }

  auto groups(field f_) const {
    if (field_kind::global == f_.kind())
      throw "invalid field kind";
    return ::boost::make_iterator_range(
               _fields.right.lower_bound(f_), _fields.right.upper_bound(f_)) |
           boost::adaptors::map_values;
  }

private:
  using group_type_fields_map = ::boost::bimaps::bimap<
      ::boost::bimaps::multiset_of<std::string>,
      ::boost::bimaps::multiset_of<field>>;

private:
  std::set<field> _global;
  group_type_fields_map _fields;
};

} // namespace prtcl::gt

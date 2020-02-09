#pragma once

#include <boost/algorithm/string/find_iterator.hpp>
#include <utility>

#include <prtcl/rt/cli/parse_args_to_ptree.hpp>

#include <boost/algorithm/string.hpp>

#include <boost/container/small_vector.hpp>

#include <boost/optional.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <boost/utility/string_view.hpp>

#include <iostream>

namespace prtcl::rt {

class command_line_interface {
  using ptree_type = decltype(rt::parse_args_to_ptree(0, nullptr));
  using path_type = typename ptree_type::path_type;
  using key_type = typename ptree_type::key_type;
  using data_type = typename ptree_type::data_type;

public:
  using name_type = key_type;

public:
  command_line_interface(int argc_, char **argv_)
      : _args{rt::parse_args_to_ptree(argc_, argv_)} {
    _positional = _args.get_child_optional("positonal");
    _name_value = _args.get_child_optional("name_value");
    _name_only = _args.get_child_optional("name_only");
  }

public:
  auto positional() const {
    return std::as_const(_args) |
           // all positional arguments are stored as 'positional' nodes
           boost::adaptors::filtered([](auto const &node) -> bool {
             return "positional" == node.first;
           }) |
           // resolve the value of each node
           boost::adaptors::transformed([](auto const &node) {
             return node.second.template get_value<data_type>();
           });
  }

public:
  bool has_name_only_argument(name_type const &name_) const {
    if (_name_only.has_value())
      return _name_only.value().get_optional<data_type>(name_).has_value();
    else
      return false;
  }

public:
  bool has_name_value_argument(name_type const &name_) const {
    if (_name_value.has_value())
      return _name_value.value().get_optional<data_type>(name_).has_value();
    else
      return false;
  }

  template <typename Type_, typename OutputIt_>
  void values_with_name(name_type const &name_, OutputIt_ it_) const {
    boost::container::small_vector<boost::string_view, 4> name_parts;

    // split the name into parts seperated by '.'
    for (auto it = boost::make_split_iterator(name_, boost::first_finder("."));
         it != decltype(it){}; ++it) {
      name_parts.emplace_back(std::addressof(*it->begin()), it->size());
    }

    if (_name_value.has_value()) {
      // create a stack and push the name_value (root) node
      boost::container::small_vector<std::pair<ptree_type const *, size_t>, 4>
          stack;
      stack.emplace_back(&_name_value.value(), 0);

      // stores the current part level
      while (not stack.empty()) {
        // take the top element of the stack
        auto [tree, level] = stack.back();
        stack.pop_back();

        // iterate over all child nodes
        for (auto const &[key, node] : *tree) {
          if (key == name_parts[level]) {
            if (level == name_parts.size() - 1)
              // if we matched the last name part, get the value
              *(it_++) = node.get_value<Type_>();
            else
              // descend deeper
              stack.emplace_back(&node, level + 1);
          }
        }
      }
    }
  }

private:
  ptree_type _args;
  boost::optional<decltype(_args) &> _positional;
  boost::optional<decltype(_args) &> _name_value;
  boost::optional<decltype(_args) &> _name_only;
};

} // namespace prtcl::rt

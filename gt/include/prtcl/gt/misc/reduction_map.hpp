#pragma once

#include "alias_to_field_map.hpp"

#include <functional>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>

#include <boost/range/adaptors.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class reduce_collection {
  // TODO: find better name for this type

  struct finder {
    // {{{
    void do_callback(std::string alias_, std::string index_) {
      /*
      std::cout << "CALLBACK ";
      if (cur_particle)
        std::cout << "P(" << cur_particle->selector_name << ", "
                  << cur_particle->index_name << ") ";
      else
        std::cout << " P(-) ";
      if (cur_neighbor)
        std::cout << "N(" << cur_neighbor->selector_name << ", "
                  << cur_neighbor->index_name << ") ";
      else
        std::cout << " N(-) ";
      std::cout << alias_ << " " << index_ << std::endl;
       */
      if (cur_particle and index_ == cur_particle->index_name)
        return callback(cur_particle->selector_name, alias_);
      callback(std::nullopt, alias_);
    }

    void operator()(ast::math::field_access const &node_) {
      do_callback(node_.field_name, node_.index_name);
    }

    void operator()(ast::math::arithmetic_nary const &node_) {
      (*this)(node_.first_operand);
      for (auto rhs : node_.right_hand_sides)
        (*this)(rhs.rhs);
    }

    void operator()(ast::math::function_call const &node_) {
      for (auto argument : node_.arguments)
        (*this)(argument);
    }

    void operator()(ast::stmt::compute const &node_) {
      do_callback(node_.field_name, node_.index_name);
      (*this)(node_.expression);
    }

    void operator()(ast::stmt::reduce const &node_) {
      do_callback(node_.field_name, node_.index_name);
      (*this)(node_.expression);
    }

    void operator()(ast::stmt::foreach_neighbor const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    void operator()(ast::stmt::foreach_particle const &node_) {
      if (cur_particle)
        throw "internal error";
      else
        cur_particle =
            item_type{node_.selector_name, node_.particle_index_name};

      /*
      std::cout << "FOREACH_PARTICLE " << node_.selector_name << " "
                << node_.particle_index_name << std::endl;
       */
      for (auto statement : node_.statements)
        (*this)(statement);

      if (not cur_particle)
        throw "internal error";
      else
        cur_particle.reset();
    }

    void operator()(ast::stmt::procedure const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    void operator()(ast::prtcl_source_file const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    template <typename T_> void operator()(T_) {
      // catch-all
    }

    // {{{ generic

    template <typename... Ts_>
    void operator()(std::variant<Ts_...> const &node_) {
      std::visit(*this, node_);
    }

    template <typename T_> void operator()(value_ptr<T_> const &node_) {
      if (not node_)
        throw "empty value_ptr";
      (*this)(*node_);
    }

    void operator()(std::monostate) { throw "monostate in ast traversal"; }

    // }}}

    std::function<void(std::optional<std::string>, ast::stmt::reduce)> callback;

    struct item_type {
      std::string selector_name;
      std::string index_name;
    };
    std::optional<item_type> cur_particle;
    // }}}
  };

  struct entry {
    std::string field_alias;
    std::string index_name;

    bool operator<(entry const &rhs) const {
      return field_alias < rhs.field_alias;
    }

    bool operator==(entry const &rhs) const {
      return field_alias == rhs.field_alias;
    }
  };

  using set_type = std::set<entry>;
  using range_type =
      decltype(std::declval<set_type>() | boost::adaptors::map_values);

public:
  range_type global() const { return _global | boost::adaptors::map_values; }

  std::optional<range_type> uniform(std::string const &key_) const {
    if (auto it = _alias_to_uniform.find(key_); it != _alias_to_uniform.end())
      return it->second | boost::adaptors::map_values;
    else
      return std::nullopt;
  }

public:
  template <typename Node_>
  friend reduce_collection make_reduce_collection(Node_ const &);

private:
  set_type _global;
  std::unordered_map<std::string, set_type> _alias_to_uniform;
};

template <typename Node_>
reduce_collection make_reduce_collection(Node_ const &node_) {
  reduce_collection result;
  reduce_collection::finder{{[&result](auto selector, auto alias) {
                              if (selector)
                                result._map[*selector].insert(alias);
                              else
                                result._map[""].insert(alias);
                            }},
                            {},
                            {}}(node_);

  return result;
}

} // namespace prtcl::gt

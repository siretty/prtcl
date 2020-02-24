#pragma once

#include "../ast.hpp"

#include <functional>
#include <optional>
#include <set>
#include <unordered_map>

#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

class particle_selector_to_fields_map {
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
      if (cur_neighbor and index_ == cur_neighbor->index_name)
        return callback(cur_neighbor->selector_name, alias_);
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
      if (cur_neighbor or not cur_particle)
        throw "internal error";
      cur_neighbor = item_type{node_.selector_name, node_.neighbor_index_name};
      /*
      std::cout << "FOREACH_NEIGHBOR " << node_.selector_name << " "
                << node_.neighbor_index_name << std::endl;
       */
      for (auto statement : node_.statements)
        (*this)(statement);
      cur_neighbor.reset();
    }

    void operator()(ast::stmt::foreach_particle const &node_) {
      if (cur_particle or cur_neighbor)
        throw "internal error";
      cur_particle = item_type{node_.selector_name, node_.particle_index_name};
      /*
      std::cout << "FOREACH_PARTICLE " << node_.selector_name << " "
                << node_.particle_index_name << std::endl;
       */
      for (auto statement : node_.statements)
        (*this)(statement);
      cur_particle.reset();
    }

    void operator()(ast::stmt::let const &node_) {
      if (std::get_if<ast::init::particle_selector>(&node_.initializer)) {
        callback(node_.alias_name, std::nullopt);
      }
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

    std::function<void(std::optional<std::string>, std::optional<std::string>)>
        callback;

    struct item_type {
      std::string selector_name;
      std::string index_name;
    };
    std::optional<item_type> cur_particle, cur_neighbor;
    // }}}
  };

public:
  bool contains(std::string const &key_) const {
    return _map.find(key_) != _map.end();
  }

  std::optional<std::set<std::string>> search(std::string const &key_) const {
    if (auto it = _map.find(key_); it != _map.end())
      return it->second;
    else
      return std::nullopt;
  }

  auto items() const { return boost::make_iterator_range(_map); }

public:
  friend particle_selector_to_fields_map
  make_particle_selector_to_fields_map(ast::prtcl_source_file const &);

private:
  std::unordered_map<std::string, std::set<std::string>> _map;
};

particle_selector_to_fields_map
make_particle_selector_to_fields_map(ast::prtcl_source_file const &node_) {
  particle_selector_to_fields_map result;

  particle_selector_to_fields_map::finder{
      {[&result](auto selector, auto alias) {
        if (selector and not alias)
          result._map[*selector];
        else {
          if (selector)
            result._map[*selector].insert(alias.value());
          else
            result._map[""].insert(alias.value());
        }
      }},
      {},
      {}}(node_);

  return result;
}

} // namespace prtcl::gt

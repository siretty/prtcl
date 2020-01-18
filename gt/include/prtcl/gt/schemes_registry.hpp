#pragma once

#include <prtcl/core/macros.hpp>

#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/dsl_to_ast.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>

#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/map.hpp>

namespace prtcl::gt {

class schemes_registry {
public:
  template <typename... Args_>
  void scheme_procedures(std::string name_, Args_ &&... args_) {
    ast::collection *co = nullptr;
    if (auto it = _schemes.find(name_); it != _schemes.end()) {
      co = it->second.get();
    } else {
      co = _schemes.emplace(name_, new ast::collection{name_})
               .first->second.get();
    }

    boost::hana::for_each(
        boost::hana::make_tuple(std::forward<Args_>(args_)...),
        [&co](auto &&dsl_) {
          co->add_child(
              dsl_to_ast(std::forward<decltype(dsl_)>(dsl_)).release());
        });
  }

  auto schemes() const {
    return _schemes | boost::adaptors::map_values | boost::adaptors::indirected;
  }

  auto &get_scheme(std::string name_) {
    if (auto it = _schemes.find(name_); it != _schemes.end())
      return *(it->second.get());
    else
      throw std::runtime_error{"unknown name"};
  }

private:
  std::unordered_map<std::string, std::unique_ptr<ast::collection>> _schemes;
};

schemes_registry &get_schemes_registry();

} // namespace prtcl::gt

#define PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(UNIQUE_ID_)                  \
  namespace PRTCL_CONCAT(                                                      \
      register_prtcl_scheme_procedures_##UNIQUE_ID_, __LINE__) {               \
    static struct register_scheme_procedures_type {                            \
      register_scheme_procedures_type() {                                      \
        auto &registry = prtcl::gt::get_schemes_registry();

#define PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES                              \
  }                                                                            \
  }                                                                            \
  register_scheme_procedures_instance;                                         \
  }

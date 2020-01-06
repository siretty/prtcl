#pragma once

#include <ostream>
#include <string>

#include <cstddef>

#include <boost/operators.hpp>

namespace prtcl::gt {

class function : public ::boost::totally_ordered<function> {
public:
  using name_type = std::string;

public:
  auto name() const { return _name; }

public:
  function() = delete;

  function(function const &) = default;
  function &operator=(function const &) = default;

  function(function &&) = default;
  function &operator=(function &&) = default;

  template <typename Name_>
  explicit function(Name_ &&name_) : _name{std::forward<Name_>(name_)} {
    // TODO: validate name
  }

public:
  bool operator==(function rhs_) const { return _name == rhs_.name(); }

  bool operator<(function rhs_) const { return name() < rhs_.name(); }

private:
  name_type _name;

  friend std::ostream &operator<<(std::ostream &o_, function const &f_) {
    o_ << "function{\"" << f_.name() << "\"}";
    return o_;
  }
};

} // namespace prtcl::gt

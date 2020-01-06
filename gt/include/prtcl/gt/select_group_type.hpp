#pragma once

#include <ostream>
#include <string>

namespace prtcl::gt {

class select_group_type {
public:
  auto group_type() const { return _group_type; }

private:
  friend std::ostream &
  operator<<(std::ostream &o_, select_group_type const &s_) {
    return o_ << "select_group_type{\"" << s_.group_type() << "\"}";
  }

public:
  select_group_type() = delete;

  select_group_type(select_group_type const &) = default;
  select_group_type &operator=(select_group_type const &) = default;

  select_group_type(select_group_type &&) = default;
  select_group_type &operator=(select_group_type &&) = default;

  explicit select_group_type(std::string group_type_)
      : _group_type{group_type_} {
    // TODO: validate group type
  }

private:
  std::string _group_type;
};

} // namespace prtcl::gt

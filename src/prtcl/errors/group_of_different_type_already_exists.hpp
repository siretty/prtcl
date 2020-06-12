#ifndef PRTCL_SRC_PRTCL_ERRORS_GROUP_OF_DIFFERENT_TYPE_ALREADY_EXISTS_HPP
#define PRTCL_SRC_PRTCL_ERRORS_GROUP_OF_DIFFERENT_TYPE_ALREADY_EXISTS_HPP

#include <stdexcept>

namespace prtcl {

class GroupOfDifferentTypeAlreadyExists : public std::exception {
  char const *what() const noexcept final {
    return "group with the same name but different group type already exists";
  }
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_ERRORS_GROUP_OF_DIFFERENT_TYPE_ALREADY_EXISTS_HPP

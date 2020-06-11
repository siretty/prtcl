#ifndef PRTCL_SRC_PRTCL_ERRORS_FIELD_OF_DIFFERENT_KIND_ALREADY_EXISTS_ERROR_HPP
#define PRTCL_SRC_PRTCL_ERRORS_FIELD_OF_DIFFERENT_KIND_ALREADY_EXISTS_ERROR_HPP

#include <stdexcept>

namespace prtcl {

class FieldOfDifferentKindAlreadyExistsError : public std::exception {
public:
  char const *what() const noexcept override {
    return "field with the same name but of different kind (uniform, varying, "
           "...) already exists";
  }
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_ERRORS_FIELD_OF_DIFFERENT_KIND_ALREADY_EXISTS_ERROR_HPP

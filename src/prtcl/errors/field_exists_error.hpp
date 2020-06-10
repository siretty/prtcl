#ifndef PRTCL_FIELD_EXISTS_ERROR_HPP
#define PRTCL_FIELD_EXISTS_ERROR_HPP

#include <stdexcept>

namespace prtcl {

class FieldExistsError : public std::exception{
public:
  char const *what() const noexcept override {
    return "field with the same name but different types already exists";
  }
};

} //

#endif // PRTCL_FIELD_EXISTS_ERROR_HPP

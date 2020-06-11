#ifndef PRTCL_SRC_PRTCL_ERRORS_FIELD_DOES_NOT_EXIST_HPP
#define PRTCL_SRC_PRTCL_ERRORS_FIELD_DOES_NOT_EXIST_HPP

#include <stdexcept>

namespace prtcl {

class FieldDoesNotExist : public std::exception {
public:
  char const *what() const noexcept final {
    return "requested field does not exists";
  }
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_ERRORS_FIELD_DOES_NOT_EXIST_HPP

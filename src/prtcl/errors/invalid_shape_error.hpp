#ifndef PRTCL_SRC_PRTCL_ERRORS_INVALID_SHAPE_ERROR_HPP
#define PRTCL_SRC_PRTCL_ERRORS_INVALID_SHAPE_ERROR_HPP

#include <stdexcept>

namespace prtcl {

class InvalidShapeError : public std::exception {
  char const *what() const noexcept final { return "invalid shape"; }
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_ERRORS_INVALID_SHAPE_ERROR_HPP

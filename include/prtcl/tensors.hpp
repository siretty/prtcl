#ifndef PRTCL_TENSORS_HPP
#define PRTCL_TENSORS_HPP

#include <typeindex>

#include <cstddef>

#include <boost/container/static_vector.hpp>

namespace prtcl {

class Tensors {
public:
  using TypeID = std::type_index;
  using ShapeType = boost::container::static_vector<size_t, 2>;

public:
  virtual ~Tensors() = default;

public:
  virtual size_t Size() const = 0;

  virtual TypeID ComponentTypeID() const = 0;

  virtual ShapeType Shape() const = 0;

public:
  virtual void Resize(size_t) = 0;

  virtual void Permute(size_t *) = 0;
};

} // namespace prtcl

#endif // PRTCL_TENSORS_HPP

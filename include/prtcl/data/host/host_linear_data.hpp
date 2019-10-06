#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>

#include <cstddef>

namespace prtcl {

template <typename T> struct host_linear_data {
  static_assert(!std::is_const<T>::value);

  size_t size_ = 0;
  std::unique_ptr<T[]> data_ = {};

public:
  size_t size() const { return size_; }

  T *data() const { return data_.get(); }

public:
  T *begin() const { return data(); }

  T *end() const { return data() + size(); }

public:
  T &operator[](size_t pos) const { return data()[pos]; }

public:
  void resize(size_t new_size) {
    data_ = std::make_unique<T[]>(new_size);
    size_ = new_size;
  }

  void resize(size_t new_size, T value) {
    data_ = std::make_unique<T[]>(new_size);
    size_ = new_size;
    std::fill(&data_[0], &data_[new_size], value);
  }
};

} // namespace prtcl

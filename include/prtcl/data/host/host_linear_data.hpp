#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

#include <cstddef>

namespace prtcl {

template <typename T> struct host_linear_data {
  static_assert(!std::is_const<T>::value);

private:
  struct impl {
    std::vector<T> data;
  };

private:
  std::shared_ptr<impl> impl_;

public:
  host_linear_data() : impl_{std::make_shared<impl>()} {}

  host_linear_data(host_linear_data const &) = default;
  host_linear_data &operator=(host_linear_data const &) = default;

  host_linear_data(host_linear_data &&) = default;
  host_linear_data &operator=(host_linear_data &&) = default;

public:
  size_t size() const { return impl_->data.size(); }

  T *data() const { return impl_->data.data(); }

public:
  T *begin() const { return data(); }

  T *end() const { return data() + size(); }

public:
  T &operator[](size_t pos) const { return data()[pos]; }

public:
  void resize(size_t new_size) {
    impl_->data.resize(new_size);
  }
};

} // namespace prtcl

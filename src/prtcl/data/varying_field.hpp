#ifndef PRTCL_SRC_PRTCL_DATA_VARYING_FIELD_HPP
#define PRTCL_SRC_PRTCL_DATA_VARYING_FIELD_HPP

#include "../cxx/span.hpp"
#include "../math.hpp"
#include "tensor_type.hpp"

#include <any>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

#include <cstddef>

#include <boost/algorithm/apply_permutation.hpp>

#include <boost/container/small_vector.hpp>

#include <boost/hana.hpp>

namespace prtcl {

template <typename T, size_t... N>
class VaryingFieldSpan {
public:
  using ItemType = math::Tensor<T, N...>;

  using reference = ItemType &;

public:
  operator bool() const { return span_.data(); }

public:
  size_t size() const { return span_.size(); }

  size_t GetSize() const { return size(); }

public:
  reference operator[](size_t index) const { return span_[index]; }

public:
  ItemType Get(size_t index) const { return (*this)[index]; }

  void Set(size_t index, ItemType const &value) const {
    (*this)[index] = value;
  }

public:
  VaryingFieldSpan() = default;

public:
  VaryingFieldSpan(cxx::span<ItemType> span) : span_{span} {}

private:
  cxx::span<ItemType> span_ = {};
};

template <typename T, size_t... N>
class VaryingFieldWrap {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  operator bool() const { return get_ and set_ and size_; }

public:
  size_t size() const { return size_(); }

  size_t GetSize() const { return size(); }

public:
  class reference {
  public:
    reference &operator=(ItemType const &value) {
      wrap_.Set(index_, value);
      return *this;
    }

    operator ItemType() { return wrap_.Get(index_); }

  public:
    reference(VaryingFieldWrap const &wrap, size_t index)
        : wrap_{wrap}, index_{index} {}

  private:
    VaryingFieldWrap const &wrap_;
    size_t index_;
  };

  reference operator[](size_t index) const { return {*this, index}; }

public:
  ItemType Get(size_t index) const { return get_(index); }

  void Set(size_t index, ItemType const &value) const {
    return set_(index, value);
  }

public:
  VaryingFieldWrap() = default;
  VaryingFieldWrap(VaryingFieldWrap const &) = delete;
  VaryingFieldWrap &operator=(VaryingFieldWrap const &) = delete;
  VaryingFieldWrap(VaryingFieldWrap &&) = default;
  VaryingFieldWrap &operator=(VaryingFieldWrap &&) = default;

public:
  template <typename Getter, typename Setter, typename Size>
  VaryingFieldWrap(Getter &&getter, Setter &&setter, Size &&size)
      : get_{std::forward<Getter>(getter)}, set_{std::forward<Setter>(setter)},
        size_{size} {}

private:
  std::function<ItemType(size_t)> get_;
  std::function<void(size_t, ItemType const &)> set_;
  std::function<size_t()> size_;
};

class VaryingFieldBase {
public:
  virtual ~VaryingFieldBase() = default;

public:
  virtual size_t GetSize() const = 0;

  virtual TensorType GetType() const = 0;

public:
  virtual void Resize(size_t new_size) = 0;

  virtual void ConsumePermutation(cxx::span<size_t> mut_perm) = 0;
};

template <typename T, size_t... N>
class VaryingFieldData : public VaryingFieldBase {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  size_t GetSize() const final { return data_.size(); }

  TensorType GetType() const final { return GetTensorTypeCRef<T, N...>(); }

public:
  void Resize(size_t new_size) final {
    data_.resize(new_size, math::zeros<T, N...>());
  }

  void ConsumePermutation(cxx::span<size_t> mut_perm) final {
    boost::algorithm::apply_permutation(data_, mut_perm);
  }

public:
  VaryingFieldSpan<T, N...> Span() { return {data_}; }

  template <typename U>
  VaryingFieldWrap<U, N...> Wrap() {
    namespace hana = boost::hana;
    auto span = Span();
    if constexpr (hana::type_c<T> == hana::type_c<U>) {
      return {
          // getter
          [span](size_t index) -> ItemType { return span[index]; },
          // setter
          [span](size_t index, ItemType const &value) { span[index] = value; },
          // size
          [span]() { return span.size(); }};
    } else {
      using WrapItemType = typename VaryingFieldWrap<U, N...>::ItemType;
      return {// getter
              [span](size_t index) -> WrapItemType {
                return math::ComponentCast<U>(span[index]);
              },
              // setter
              [span](size_t index, WrapItemType const &value) {
                span[index] = math::ComponentCast<T>(value);
              },
              // size
              [span]() { return span.size(); }};
    }
  }

private:
  boost::container::small_vector<ItemType, 0> data_;
};

namespace detail {

template <typename T, typename... U>
constexpr auto kTypeMapping = boost::hana::make_pair(
    boost::hana::type_c<T>, boost::hana::make_tuple(boost::hana::type_c<U>...));

constexpr auto kComponentConvertibleFrom = boost::hana::make_map(
    kTypeMapping<bool, bool>, kTypeMapping<int32_t, int32_t, int64_t>,
    kTypeMapping<int64_t, int64_t, int32_t>, kTypeMapping<float, float, double>,
    kTypeMapping<double, double, float>);

} // namespace detail

class VaryingField;

template <typename T, size_t... N>
VaryingField *NewVaryingField();

template <typename T, size_t... N>
VaryingField MakeVaryingField();

class VaryingField {
  template <typename T, size_t... N>
  VaryingFieldData<T, N...> *DataAsPtr() const {
    return dynamic_cast<VaryingFieldData<T, N...> *>(data_.get());
  }

public:
  template <typename T, size_t... N>
  bool Is() const {
    return nullptr != DataAsPtr<T, N...>();
  }

public:
  size_t GetSize() const { return data_->GetSize(); }

  TensorType GetType() const { return data_->GetType(); }

public:
  void Resize(size_t new_size) { data_->Resize(new_size); }

  void ConsumePermutation(cxx::span<size_t> mut_perm) {
    data_->ConsumePermutation(mut_perm);
  }

public:
  template <typename T, size_t... N>
  VaryingFieldSpan<T, N...> Span() {
    if (auto *ptr = DataAsPtr<T, N...>())
      return ptr->Span();
    else
      return {};
  }

  template <typename U, size_t... N>
  VaryingFieldWrap<U, N...> Wrap() {
    namespace hana = boost::hana;

    constexpr auto ctypes =
        hana::find(detail::kComponentConvertibleFrom, hana::type_c<U>);
    if constexpr (ctypes != hana::nothing) {
      std::optional<VaryingFieldWrap<U, N...>> result;

      hana::for_each(ctypes.value(), [this, &result](auto ctype) {
        using T = typename decltype(ctype)::type;
        if (auto *ptr = this->DataAsPtr<T, N...>())
          result = ptr->template Wrap<U>();
      });

      if (result)
        return std::move(*result);
      else
        return {};
    } else
      return {};
  }

public:
  template <typename T, size_t... N>
  friend VaryingField *NewVaryingField();

  template <typename T, size_t... N>
  friend VaryingField MakeVaryingField();

private:
  VaryingField(VaryingFieldBase *data) : data_{data} {}

private:
  std::unique_ptr<VaryingFieldBase> data_;
};

template <typename T, size_t... N>
VaryingField *NewVaryingField() {
  return new VaryingField{new VaryingFieldData<T, N...>};
}

template <typename T, size_t... N>
VaryingField MakeVaryingField() {
  return VaryingField{new VaryingFieldData<T, N...>};
}

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_VARYING_FIELD_HPP

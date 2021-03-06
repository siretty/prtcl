#ifndef PRTCL_SRC_PRTCL_DATA_UNIFORM_FIELD_HPP
#define PRTCL_SRC_PRTCL_DATA_UNIFORM_FIELD_HPP

#include "../cxx/span.hpp"
#include "../math.hpp"
#include "../util/archive.hpp"
#include "tensor_type.hpp"

#include <any>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

#include <cstddef>

#include <boost/hana.hpp>

namespace prtcl {

template <typename T, size_t... N>
class UniformFieldSpan {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  operator bool() const { return item_ptr_; }

public:
  UniformFieldSpan &operator=(ItemType const &value) {
    (*item_ptr_) = value;
    return *this;
  }

  operator ItemType() { return (*item_ptr_); }

public:
  ItemType &operator*() const { return *item_ptr_; }

public:
  ItemType Get() const { return (*item_ptr_); }

  void Set(ItemType const &value) const { (*item_ptr_) = value; }

public:
  UniformFieldSpan() = default;

public:
  UniformFieldSpan(ItemType *item_ptr) : item_ptr_{item_ptr} {}

private:
  ItemType *item_ptr_ = nullptr;
};

template <typename T, size_t... N>
class UniformFieldWrap {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  operator bool() const { return get_ and set_; }

public:
  UniformFieldWrap &operator=(ItemType const &value) {
    Set(value);
    return *this;
  }

  operator ItemType() { return Get(); }

public:
  ItemType Get() const { return get_(); }

  void Set(ItemType const &value) const { return set_(value); }

public:
  UniformFieldWrap() = default;
  UniformFieldWrap(UniformFieldWrap const &) = delete;
  UniformFieldWrap &operator=(UniformFieldWrap const &) = delete;
  UniformFieldWrap(UniformFieldWrap &&) = default;
  UniformFieldWrap &operator=(UniformFieldWrap &&) = default;

public:
  template <typename Getter, typename Setter>
  UniformFieldWrap(Getter &&getter, Setter &&setter)
      : get_{std::forward<Getter>(getter)}, set_{std::forward<Setter>(setter)} {
  }

private:
  std::function<ItemType()> get_;
  std::function<void(ItemType const &)> set_;
};

class UniformFieldBase {
public:
  virtual ~UniformFieldBase() = default;

public:
  virtual TensorType GetType() const = 0;

public:
  virtual void GetScalar(RealScalar &scalar) const = 0;

  virtual void GetVector(RealVector &vector) const = 0;

  virtual void GetMatrix(RealMatrix &matrix) const = 0;

  virtual void GetScalar(IntegerScalar &scalar) const = 0;

  virtual void GetVector(IntegerVector &vector) const = 0;

  virtual void GetMatrix(IntegerMatrix &matrix) const = 0;

  virtual void GetScalar(BooleanScalar &scalar) const = 0;

  virtual void GetVector(BooleanVector &vector) const = 0;

  virtual void GetMatrix(BooleanMatrix &matrix) const = 0;

public:
  virtual void SetScalar(RealScalar const &scalar) = 0;

  virtual void SetVector(RealVector const &vector) = 0;

  virtual void SetMatrix(RealMatrix const &matrix) = 0;

  virtual void SetScalar(IntegerScalar const &scalar) = 0;

  virtual void SetVector(IntegerVector const &vector) = 0;

  virtual void SetMatrix(IntegerMatrix const &matrix) = 0;

  virtual void SetScalar(BooleanScalar const &scalar) = 0;

  virtual void SetVector(BooleanVector const &vector) = 0;

  virtual void SetMatrix(BooleanMatrix const &matrix) = 0;

public:
  virtual void Save(ArchiveWriter &archive) const = 0;

  virtual void Load(ArchiveReader &archive) = 0;
};

template <typename T, size_t... N>
class UniformFieldData : public UniformFieldBase {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  TensorType GetType() const final { return GetTensorTypeCRef<T, N...>(); }

private:
  template <size_t Rank, typename OutComp, typename OutItem>
  void GetImpl(OutItem &out) const {
    if constexpr (Rank == sizeof...(N)) {
      if constexpr (IsComponentConvertible<T, OutComp>())
        out = math::ComponentCast<OutComp>(data_);
      else
        throw "INVALID TYPE";
    } else
      throw "INVALID SHAPE";
  }

public:
  void GetScalar(RealScalar &scalar) const final {
    GetImpl<0, RealScalar>(scalar);
  }

  void GetVector(RealVector &vector) const final {
    GetImpl<1, RealScalar>(vector);
  }

  void GetMatrix(RealMatrix &matrix) const final {
    GetImpl<2, RealScalar>(matrix);
  }

  void GetScalar(IntegerScalar &scalar) const final {
    GetImpl<0, IntegerScalar>(scalar);
  }

  void GetVector(IntegerVector &vector) const final {
    GetImpl<1, IntegerScalar>(vector);
  }

  void GetMatrix(IntegerMatrix &matrix) const final {
    GetImpl<2, IntegerScalar>(matrix);
  }

  void GetScalar(BooleanScalar &scalar) const final {
    GetImpl<0, BooleanScalar>(scalar);
  }

  void GetVector(BooleanVector &vector) const final {
    GetImpl<1, BooleanScalar>(vector);
  }

  void GetMatrix(BooleanMatrix &matrix) const final {
    GetImpl<2, BooleanScalar>(matrix);
  }

private:
  template <size_t Rank, typename InpComp, typename InpItem>
  void SetImpl(InpItem const &inp) {
    if constexpr (Rank == sizeof...(N)) {
      if constexpr (IsComponentConvertible<InpComp, T>())
        data_ = math::ComponentCast<T>(inp);
      else
        throw "INVALID TYPE";
    } else
      throw "INVALID SHAPE";
  }

public:
  void SetScalar(RealScalar const &scalar) final {
    SetImpl<0, RealScalar>(scalar);
  }

  void SetVector(RealVector const &vector) final {
    SetImpl<1, RealScalar>(vector);
  }

  void SetMatrix(RealMatrix const &matrix) final {
    SetImpl<2, RealScalar>(matrix);
  }

  void SetScalar(IntegerScalar const &scalar) final {
    SetImpl<0, IntegerScalar>(scalar);
  }

  void SetVector(IntegerVector const &vector) final {
    SetImpl<1, IntegerScalar>(vector);
  }

  void SetMatrix(IntegerMatrix const &matrix) final {
    SetImpl<2, IntegerScalar>(matrix);
  }

  void SetScalar(BooleanScalar const &scalar) final {
    SetImpl<0, BooleanScalar>(scalar);
  }

  void SetVector(BooleanVector const &vector) final {
    SetImpl<1, BooleanScalar>(vector);
  }

  void SetMatrix(BooleanMatrix const &matrix) final {
    SetImpl<2, BooleanScalar>(matrix);
  }

public:
  void Save(ArchiveWriter &archive) const final {
    if constexpr (sizeof...(N) == 0)
      archive.SaveValues(1, &data_);
    else {
      auto const component_count = static_cast<size_t>(ItemType{}.size());
      archive.SaveValues(component_count, data_.data());
    }
  }

  void Load(ArchiveReader &archive) {
    if constexpr (sizeof...(N) == 0)
      archive.LoadValues(1, &data_);
    else {
      auto const component_count = static_cast<size_t>(ItemType{}.size());
      archive.LoadValues(component_count, data_.data());
    }
  }

public:
  UniformFieldSpan<T, N...> Span() { return {&data_}; }

  template <typename U>
  UniformFieldWrap<U, N...> Wrap() {
    namespace hana = boost::hana;
    if constexpr (hana::type_c<T> == hana::type_c<U>) {
      return {
          // getter
          [this]() -> ItemType { return data_; },
          // setter
          [this](ItemType const &value) mutable { data_ = value; },
      };
    } else {
      using WrapItemType = typename UniformFieldWrap<U, N...>::ItemType;
      return {
          // getter
          [this]() -> WrapItemType { return math::ComponentCast<U>(data_); },
          // setter
          [this](WrapItemType const &value) mutable {
            data_ = math::ComponentCast<T>(value);
          },
      };
    }
  }

private:
  ItemType data_;
};

class UniformField;

template <typename T, size_t... N>
UniformField MakeUniformField();

class UniformField {
  template <typename T, size_t... N>
  UniformFieldData<T, N...> *DataAsPtr() const {
    return dynamic_cast<UniformFieldData<T, N...> *>(data_.get());
  }

public:
  template <typename T, size_t... N>
  bool Is() const {
    return nullptr != DataAsPtr<T, N...>();
  }

public:
  TensorType GetType() const { return data_->GetType(); }

public:
  void Get(RealScalar &scalar) const { data_->GetScalar(scalar); }

  void Get(RealVector &vector) const { data_->GetVector(vector); }

  void Get(RealMatrix &matrix) const { data_->GetMatrix(matrix); }

  void Get(IntegerScalar &scalar) const { data_->GetScalar(scalar); }

  void Get(IntegerVector &vector) const { data_->GetVector(vector); }

  void Get(IntegerMatrix &matrix) const { data_->GetMatrix(matrix); }

  void Get(BooleanScalar &scalar) const { data_->GetScalar(scalar); }

  void Get(BooleanVector &vector) const { data_->GetVector(vector); }

  void Get(BooleanMatrix &matrix) const { data_->GetMatrix(matrix); }

public:
  void Set(RealScalar const &scalar) const { data_->SetScalar(scalar); }

  void Set(RealVector const &vector) const { data_->SetVector(vector); }

  void Set(RealMatrix const &matrix) const { data_->SetMatrix(matrix); }

  void Set(IntegerScalar const &scalar) const { data_->SetScalar(scalar); }

  void Set(IntegerVector const &vector) const { data_->SetVector(vector); }

  void Set(IntegerMatrix const &matrix) const { data_->SetMatrix(matrix); }

  void Set(BooleanScalar const &scalar) const { data_->SetScalar(scalar); }

  void Set(BooleanVector const &vector) const { data_->SetVector(vector); }

  void Set(BooleanMatrix const &matrix) const { data_->SetMatrix(matrix); }

public:
  void Save(ArchiveWriter &archive) const { data_->Save(archive); }

  void Load(ArchiveReader &archive) const { data_->Load(archive); }

public:
  template <typename T, size_t... N>
  UniformFieldSpan<T, N...> Span() const {
    if (auto *ptr = DataAsPtr<T, N...>())
      return ptr->Span();
    else
      return {};
  }

  template <typename U, size_t... N>
  UniformFieldWrap<U, N...> Wrap() const {
    namespace hana = boost::hana;

    constexpr auto ctypes =
        hana::find(detail::kComponentConvertibleFrom, hana::type_c<U>);
    if constexpr (ctypes != hana::nothing) {
      std::optional<UniformFieldWrap<U, N...>> result;

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
  friend UniformField MakeUniformField();

public:
  UniformField() = delete;
  UniformField(UniformField const &) = default;
  UniformField &operator=(UniformField const &) = default;
  UniformField(UniformField &&) = default;
  UniformField &operator=(UniformField &&) = default;

private:
  UniformField(UniformFieldBase *data) : data_{data} {}

private:
  std::shared_ptr<UniformFieldBase> data_;
};

template <typename T, size_t... N>
UniformField MakeUniformField() {
  return UniformField{new UniformFieldData<T, N...>};
}

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_UNIFORM_FIELD_HPP

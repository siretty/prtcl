#include <array>
#include <functional>
#include <any>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <typeinfo>

#include <cstdint>

#include <boost/hana.hpp>

namespace hana = boost::hana;

template <typename ... T>
constexpr auto type_set_v = hana::make_set(hana::type_c<T>...);

template <typename T, typename ...U>
constexpr auto type_mapping_v = hana::make_pair(hana::type_c<T>, hana::make_tuple(hana::type_c<U>...));

constexpr auto allowed_types = type_set_v<bool, int32_t, int64_t, float, double>;

constexpr auto allowed_conversions = hana::make_map(
    //type_mapping_v<bool>,
    type_mapping_v<int32_t, int64_t>,
    type_mapping_v<int64_t, int32_t>,
    type_mapping_v<float, double>,
    type_mapping_v<double, float>
);

struct NotTheRightTypeError : std::exception {
  char const * what() const noexcept override {
    return "not the right type";
  }
};

struct NotTheRightSizeError : std::exception {
  char const * what() const noexcept override {
    return "not the right size";
  }
};

struct NotTheRightTypeOrSizeError : std::exception {
  char const * what() const noexcept override {
    return "not the right type or size";
  }
};

struct NotAnAllowedConversion : std::exception {
  char const *what() const noexcept override {
    return "not an allowed conversion";
  }
};

template <typename T, size_t N> struct ArraySpan {
  T Get(size_t index) const { return data[index]; }
  void Set(size_t index, T const &value) const { data[index] = value; }

  T *data;
};

template <typename T, size_t N> struct ArrayWrap {
  std::function<T(size_t)> Get;
  std::function<void(size_t, T const &)> Set;
};

template <typename T, size_t N> struct ArrayData {
  ArraySpan<T, N> GetSpan() { return {data.data()}; }

  std::array<T, N> data;
};

struct Array {
  template <typename T, size_t N> ArraySpan<T, N> GetSpan() {
    if (auto ptr = std::any_cast<ArrayData<T, N>>(&data))
      return {ptr->data.data()};
    else
      throw NotTheRightTypeError{};
  }

  template <typename T, size_t N> ArrayWrap<T, N> GetWrap() {
    namespace hana = boost::hana;

    // if requesting the exact data type, build a non-converting wrapper
    if (auto *ptr = std::any_cast<ArrayData<T, N>>(&data))
      return {
          [span=ptr->GetSpan()](size_t index) { return span.Get(index); },
          [span=ptr->GetSpan()](size_t index, T const &value) { return span.Set(index, value); }};

    // check if there are any
    constexpr auto possible_types = hana::find(allowed_conversions, hana::type_c<T>);
    if constexpr (possible_types != hana::nothing) {
      std::optional<ArrayWrap<T, N>> result;

      hana::for_each(possible_types.value(), [&data=data, &result](auto possible_type) {
        using U = typename decltype(possible_type)::type;
        if (auto *ptr = std::any_cast<ArrayData<U, N>>(&data))
          result = {
              [span=ptr->GetSpan()](size_t index) { return static_cast<U>(span.Get(index)); },
              [span=ptr->GetSpan()](size_t index, T const &value) { return span.Set(index, static_cast<T>(value)); }
          };
      });

      if (result)
        return result.value();
      else
        throw NotTheRightSizeError{};
    } else
      throw NotAnAllowedConversion{};
  }

  std::any data;
};

template <typename T, size_t N> Array MakeArray() {
  static_assert(hana::contains(allowed_types, hana::type_c<T>));
  return {ArrayData<T, N>{}};
}

int main() {
  // auto invalid = MakeArray<char, 5>(); // static_assert triggered

  auto int5_array = MakeArray<int, 5>();

  try {
    auto a = int5_array.GetSpan<float, 3>();
  } catch (NotTheRightTypeError) {
    std::cout << "good: invalid type was caught" << '\n';
  }

  try {
    auto a = int5_array.GetSpan<int, 5>();
    for (size_t i = 0; i < 5; ++i)
      a.Set(i, static_cast<int>(i));
  } catch (NotTheRightTypeError) {
    std::cout << "bad: error on valid type" << '\n';
  }

  try {
    auto a = int5_array.GetWrap<bool, 5>();
  } catch (NotAnAllowedConversion) {
    std::cout << "good: invalid conversion was caught" << '\n';
  }

  try {
    auto a = int5_array.GetWrap<int, 4>();
  } catch (NotTheRightSizeError) {
    std::cout << "good: invalid conversion with wrong size was caught" << '\n';
  }

  try {
    auto a = int5_array.GetWrap<int32_t, 5>();
    for (size_t i = 0; i < 5; ++i) {
      auto value = a.Get(i);
      std::cout << "#" << i << " = " << a.Get(i) << ' ' << typeid(decltype(value)).name() << '\n';
    }
  } catch (std::exception) {
    std::cout << "bad: error on valid conversion" << '\n';
  }

  try {
    auto a = int5_array.GetWrap<int64_t, 5>();
    for (size_t i = 0; i < 5; ++i) {
      auto value = a.Get(i);
      std::cout << "#" << i << " = " << a.Get(i) << ' ' << typeid(decltype(value)).name() << '\n';
    }
  } catch (std::exception) {
    std::cout << "bad: error on valid conversion" << '\n';
  }
}

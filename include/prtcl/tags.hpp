#pragma once

#include "meta/is_any_of.hpp"
#include "meta/remove_cvref.hpp"

#include <ostream>

#define PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(NAME)                                  \
  friend std::ostream &operator<<(std::ostream &s, NAME) { return s << #NAME; }

namespace prtcl {
namespace tag {

struct unspecified {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(unspecified)
};

template <typename T>
struct is_unspecified_tag : std::is_same<remove_cvref_t<T>, unspecified> {};

template <typename T>
constexpr bool is_unspecified_tag_v = is_unspecified_tag<T>::value;

// group

struct active {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(active)
};

struct passive {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(passive)
};

template <typename T>
struct is_group_tag : is_any_of<remove_cvref_t<T>, active, passive> {};

template <typename T> constexpr bool is_group_tag_v = is_group_tag<T>::value;

// field kind

struct uniform {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(uniform)
};

struct varying {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(varying)
};

template <typename T>
struct is_kind_tag : is_any_of<remove_cvref_t<T>, uniform, varying> {};

template <typename T> constexpr bool is_kind_tag_v = is_kind_tag<T>::value;

// field type

struct scalar {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(scalar)
};

struct vector {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(vector)
};

template <typename T>
struct is_type_tag : is_any_of<remove_cvref_t<T>, scalar, vector> {};

template <typename T> constexpr bool is_type_tag_v = is_type_tag<T>::value;

// execution mode

struct host {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(host)
};

struct sycl {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(sycl)
};

template <typename T>
struct is_execution_tag : is_any_of<remove_cvref_t<T>, host, sycl> {};

template <typename T>
constexpr bool is_execution_tag_v = is_execution_tag<T>::value;

// functions

struct dot {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(dot)
};

struct norm {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(norm)
};

struct norm_squared {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(norm_squared)
};

struct normalized {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(normalized)
};

struct min {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(min)
};

struct max {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(max)
};

struct kernel {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(kernel)
};

struct kernel_gradient {
  PRTCL_DEFINE_TAG_OSTREAM_LSHIFT(kernel_gradient)
};

template <typename T>
struct is_function_tag
    : is_any_of<remove_cvref_t<T>, dot, norm, norm_squared, normalized, min,
                max, kernel, kernel_gradient> {};

template <typename T>
constexpr bool is_function_tag_v = is_function_tag<T>::value;

} // namespace tag
} // namespace prtcl

#undef PRTCL_DEFINE_TAG_OSTREAM_LSHIFT

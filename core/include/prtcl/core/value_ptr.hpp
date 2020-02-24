#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace prtcl::core {

struct bad_value_ptr_access : std::exception {};

template <typename ValueType_> class value_ptr {
public:
  using value_type = ValueType_;

public:
  value_ptr() = default;

  value_ptr(value_ptr &&) = default;
  value_ptr &operator=(value_ptr &&) = default;

public:
  value_ptr(value_ptr const &other_) : _ptr{other_._copy_ptr()} {}

  value_ptr &operator=(value_ptr const &other_) {
    if (this != &other_)
      _ptr = other_._copy_ptr();
    return *this;
  }

public:
  value_ptr(value_type const &value_)
      : _ptr{std::make_unique<value_type>(value_)} {}

  value_ptr(value_type &&value_)
      : _ptr{std::make_unique<value_type>(std::move(value_))} {}

private:
  template <typename T_>
  static constexpr bool _ctor_ok = not std::is_same<T_, value_type>::value and
                                   not std::is_same<T_, value_ptr>::value;

public:
  template <typename T_, typename = std::enable_if_t<_ctor_ok<T_>>>
  value_ptr(T_ const &t_) : _ptr{std::make_unique<value_type>(t_)} {}

  template <typename T_, typename = std::enable_if_t<_ctor_ok<T_>>>
  value_ptr &operator=(T_ const &t_) {
    return *this = value_ptr{t_};
  }

public:
  void swap(value_ptr &other_) { std::swap(_ptr, other_._ptr); }

public:
  bool empty() const { return not static_cast<bool>(_ptr); }

  decltype(auto) get() { return _ptr.get(); }
  decltype(auto) get() const { return _ptr.get(); }

  decltype(auto) value() { return *_filled_self(); }
  decltype(auto) value() const { return *_filled_self(); }

  // TODO: value_or(...)

public:
  operator bool() const { return static_cast<bool>(_ptr); }

  decltype(auto) operator*() { return *_ptr; }
  decltype(auto) operator*() const { return *_ptr; }

  decltype(auto) operator-> () { return _ptr.operator->(); }
  decltype(auto) operator-> () const { return _ptr.operator->(); }

private:
  auto _copy_ptr() const { return std::make_unique<value_type>(*_ptr); }

  // {{{ _filled_self() [const] -> value_ptr [const] &

private:
  value_ptr &_filled_self() {
    return const_cast<value_ptr const &>(std::as_const(*this)._filled_self());
  }

  value_ptr const &_filled_self() const {
    if (empty())
      throw bad_value_ptr_access{};
    return *this;
  }

  // }}}

private:
  std::unique_ptr<value_type> _ptr;
};

} // namespace prtcl::core

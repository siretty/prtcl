#pragma once

#include "../common.hpp"

#include <ostream>
#include <stdexcept>
#include <variant>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/type_index.hpp>

namespace prtcl::gt {

struct ast_printer_error : std::exception {
  const char *what() const noexcept override {
    return "fatal ast printer error";
  }
};

template <typename Impl_> struct printer_crtp {
private:
  using impl_type = Impl_;

protected:
  using base_type = printer_crtp;

protected:
  template <typename CharT_, typename Traits_>
  static std::basic_ostream<CharT_, Traits_> &
  nl(std::basic_ostream<CharT_, Traits_> &stream_) {
    return stream_ << std::endl;
  }

  template <typename Range_, typename Separator_> struct sep_type {
    template <typename CharT_, typename Traits_>
    friend std::basic_ostream<CharT_, Traits_> &operator<<(
        std::basic_ostream<CharT_, Traits_> &stream_, sep_type const &self_) {
      auto first = boost::begin(self_._range);
      auto last = boost::end(self_._range);
      if (first != last) {
        self_._impl(*first);
        ++first;
      }
      for (; first != last; ++first) {
        stream_ << self_._separator;
        self_._impl(*first);
      }
      return stream_;
    }

    impl_type &_impl;
    Range_ const &_range;
    Separator_ const &_separator;
  };

  template <typename Range_, typename Separator_>
  auto sep(Range_ const &range_, Separator_ const &separator_) {
    return sep_type<Range_, Separator_>{impl(), range_, separator_};
  }

  // {{{ join(range, separator[, transform])

  template <
      typename Range_, typename Separator_, typename Empty_,
      typename Transform_>
  struct join_type {
    template <typename CharT_, typename Traits_>
    friend std::basic_ostream<CharT_, Traits_> &operator<<(
        std::basic_ostream<CharT_, Traits_> &stream_, join_type const &self_) {
      auto first = boost::begin(self_._range);
      auto last = boost::end(self_._range);
      if (first == last) {
        // range is empty
        stream_ << self_._empty;
      } else {
        // range is not empty
        stream_ << self_._transform(*first);
        ++first;
        for (; first != last; ++first) {
          stream_ << self_._separator;
          stream_ << self_._transform(*first);
        }
      }
      return stream_;
    }

    Range_ const &_range;
    Separator_ const &_separator;
    Empty_ const &_empty;
    Transform_ _transform;
  };

  template <
      typename Range_, typename Separator_, typename Empty_,
      typename Transform_>
  static auto join(
      Range_ const &range_, Separator_ const &separator_, Empty_ const &empty_,
      Transform_ transform_) {
    return join_type<Range_, Separator_, Empty_, Transform_>{
        range_, separator_, empty_, transform_};
  }

  template <typename Range_, typename Separator_, typename Empty_>
  static auto join(
      Range_ const &range_, Separator_ const &separator_,
      Empty_ const &empty_) {
    auto identity = [](auto &&x) -> decltype(auto) {
      return std::forward<decltype(x)>(x);
    };
    return join(range_, separator_, empty_, identity);
  }

  // }}}

  struct indent_type {
    template <typename CharT_, typename Traits_>
    friend std::basic_ostream<CharT_, Traits_> &operator<<(
        std::basic_ostream<CharT_, Traits_> &stream_,
        indent_type const &indent_) {
      for (size_t i = 0; i < indent_.indent * indent_.indent_width; ++i)
        stream_ << ' ';
      return stream_;
    }

    size_t indent;
    size_t indent_width;
  };

  auto &out() { return _out; }
  auto &outi() { return _out << indent(); }

  auto indent() const { return indent_type{_indent, _indent_width}; }

  void increase_indent() { ++_indent; }

  void decrease_indent() {
    if (_indent == 0)
      throw "zero indent cannot be decreased";
    --_indent;
  }

public:
  //! Unpack the contents of a variant<...>.
  template <typename... Ts_> void operator()(std::variant<Ts_...> const &arg_) {
    std::visit(impl(), arg_);
  }

  //! Unpack the contents of a value_ptr<...>.
  /// Throws if the value_ptr is empty.
  template <typename ValueType_>
  void operator()(prtcl::gt::value_ptr<ValueType_> const &arg_) {
    if (not arg_)
      throw "empty value_ptr";
    impl()(*arg_);
  }

  void operator()(std::string const &arg_) { out() << arg_; }

  //! Always throws since it is only called for empty variants.
  void operator()(std::monostate) { throw "empty expression"; }

  //! Notify the user that printing of some type is not implemented.
  template <typename Arg_> void operator()(Arg_) {
    out() << "(not implemented: "
          << boost::typeindex::type_id<Arg_>().pretty_name() << ")";
  }

private:
  Impl_ &impl() { return *static_cast<Impl_ *>(this); }
  Impl_ const &impl() const { return *static_cast<Impl_ const *>(this); }

public:
  printer_crtp(std::ostream &stream_) : _out{stream_} {}

private:
  std::ostream &_out;
  size_t _indent_width = 2;
  size_t _indent = 0;
};

} // namespace prtcl::gt

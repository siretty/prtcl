#pragma once

#include <limits>
#include <utility>

#include <cstddef>

#include <eigen3/Eigen/Eigen>

namespace prtcl {

template <typename MathTraits> struct dot_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::dot(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::dot(std::forward<Args>(args)...);
  }
};

template <typename MathTraits> struct norm_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::norm(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::norm(std::forward<Args>(args)...);
  }
};

template <typename MathTraits> struct norm_squared_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::norm_squared(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::norm_squared(std::forward<Args>(args)...);
  }
};

template <typename MathTraits> struct normalized_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::normalized(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::normalized(std::forward<Args>(args)...);
  }
};

template <typename MathTraits> struct max_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::max(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::max(std::forward<Args>(args)...);
  }
};

template <typename MathTraits> struct min_eval {
  template <typename> struct result;

  template <typename This, typename... Args> struct result<This(Args...)> {
    using type = decltype(MathTraits::min(std::declval<Args>()...));
  };

  template <typename... Args> auto operator()(Args &&... args) const {
    return MathTraits::min(std::forward<Args>(args)...);
  }
};

template <typename T, size_t N> struct host_math_traits {
  using scalar_type = T;
  using vector_type = Eigen::Matrix<T, static_cast<int>(N), 1>;

  constexpr static size_t vector_extent = N;

  template <typename LHS, typename RHS> static auto dot(LHS &&lhs, RHS &&rhs) {
    return std::forward<LHS>(lhs).matrix().dot(std::forward<RHS>(rhs).matrix());
  }

  template <typename Arg> static auto norm(Arg &&arg) {
    return std::forward<Arg>(arg).matrix().norm();
  }

  template <typename Arg> static auto norm_squared(Arg &&arg) {
    return dot(std::forward<Arg>(arg), std::forward<Arg>(arg));
  }

  template <typename Arg> static auto normalized(Arg &&arg) {
    return std::forward<Arg>(arg).matrix().normalized();
  }

  template <typename LHS, typename RHS> static auto max(LHS &&lhs, RHS &&rhs) {
    return std::max<scalar_type>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
  }

  template <typename LHS, typename RHS> static auto min(LHS &&lhs, RHS &&rhs) {
    return std::min<scalar_type>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
  }

  static auto make_zero_vector() { return vector_type::Zero(); }

  static scalar_type epsilon() {
    return std::numeric_limits<scalar_type>::epsilon();
  }
};

} // namespace prtcl

#pragma once

#include <prtcl/data/group.hpp>
#include <prtcl/data/openmp/uniforms.hpp>
#include <prtcl/data/openmp/varyings.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <type_traits>

#include <boost/hana.hpp>
#include <utility>

namespace prtcl::data::openmp {

template <typename Scalar, size_t N> class group {
public:
  group() = delete;

  group(group const &) = default;
  group &operator=(group const &) = default;

  group(group &&) = default;
  group &operator=(group &&) = default;

  explicit group(::prtcl::data::group<Scalar, N> &group_)
      : _size{group_.size()}, _fields{_make_field_map(group_)} {
    auto &vvs = group_.get(tag::kind::varying{}, tag::type::vector{});
    if (auto position_index = vvs.get_index("position"))
      _hack_position = &(get(tag::kind::varying{},
                             tag::type::vector{})[position_index.value()]);
  }

public:
  size_t size() const { return _size; }

  // get(tag::[uniform,varying], tag::[scalar,vector,matrix]) [const] -> ... {{{

  template <
      typename KT, typename TT,
      typename = std::enable_if_t<tag::is_kind_v<KT> and tag::is_type_v<TT>>>
  auto &get(KT &&, TT &&) {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<KT>>]
                  [boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  template <
      typename KT, typename TT,
      typename = std::enable_if_t<tag::is_kind_v<KT> and tag::is_type_v<TT>>>
  auto &get(KT &&, TT &&) const {
    return _fields[boost::hana::type_c<meta::remove_cvref_t<KT>>]
                  [boost::hana::type_c<meta::remove_cvref_t<TT>>];
  }

  // }}}

private:
  // {{{ _make_field_map(data::group &) -> ...

  static auto _make_field_map(::prtcl::data::group<Scalar, N> &group_) {
    using boost::hana::make_pair;
    using boost::hana::type_c;
    constexpr auto scalar_c = type_c<tag::type::scalar>;
    constexpr auto vector_c = type_c<tag::type::vector>;
    constexpr auto matrix_c = type_c<tag::type::matrix>;
    return boost::hana::make_map(
        make_pair(
            type_c<tag::kind::uniform>,
            boost::hana::make_map(
                make_pair(scalar_c,
                          uniforms_t<Scalar>{group_.get(tag::kind::uniform{},
                                                        tag::type::scalar{})}),
                make_pair(vector_c,
                          uniforms_t<Scalar, N>{group_.get(
                              tag::kind::uniform{}, tag::type::vector{})}),
                make_pair(matrix_c,
                          uniforms_t<Scalar, N, N>{group_.get(
                              tag::kind::uniform{}, tag::type::matrix{})}))),
        make_pair(
            type_c<tag::kind::varying>,
            boost::hana::make_map(
                make_pair(scalar_c,
                          varyings_t<Scalar>{group_.get(tag::kind::varying{},
                                                        tag::type::scalar{})}),
                make_pair(vector_c,
                          varyings_t<Scalar, N>{group_.get(
                              tag::kind::varying{}, tag::type::vector{})}),
                make_pair(matrix_c,
                          varyings_t<Scalar, N, N>{group_.get(
                              tag::kind::varying{}, tag::type::matrix{})}))));
  }

  // }}}

  using field_map_type = decltype(
      _make_field_map(std::declval<::prtcl::data::group<Scalar, N> &>()));

private:
  size_t _size = 0;
  field_map_type _fields;

public:
  tensors<Scalar, std::index_sequence<N>> const *_hack_position = nullptr;
};

template <typename Scalar, size_t N>
group(::prtcl::data::group<Scalar, N> &)->group<Scalar, N>;

// make compatible with ..._uniform_grid (this is a bit hacky)

template <typename Scalar, size_t N>
auto get_element_count(group<Scalar, N> const &group_) {
  return group_.size();
}

template <typename Scalar, size_t N>
decltype(auto) get_element_ref(group<Scalar, N> const &group_, size_t index_) {
  return (*group_._hack_position)[index_];
}

} // namespace prtcl::data::openmp

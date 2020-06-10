#ifndef PRTCL_GROUP_HPP
#define PRTCL_GROUP_HPP

#include <prtcl/errors/field_exists_error.hpp>
#include <prtcl/errors/invalid_identifier_error.hpp>
#include <prtcl/tensors.hpp>
#include <prtcl/vector_of_tensors.hpp>

#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace prtcl {

class Group {
public:
  Group() = delete;

  Group(Group &&) = default;
  Group &operator=(Group &&) = default;

  Group(std::string name, std::string type) : name_{name}, type_{type} {
    if (not IsValidIdentifier(name_))
      throw InvalidIdentifierError{};
    if (not IsValidIdentifier(type_))
      throw InvalidIdentifierError{};
  }

public:
  size_t Size() const { return size_; }

public:
  std::string_view GroupName() const { return name_; }

  std::string_view GroupType() const { return type_; }

public:
  auto Tags() const { return boost::make_iterator_range(tags_); }

  bool HasTag(std::string const &tag) const {
    return tags_.find(tag) != tags_.end();
  }

  auto &AddTag(std::string const &tag) {
    tags_.insert(tag);
    return *this;
  }

  auto &RemoveTag(std::string const &tag) {
    tags_.erase(tag);
    return *this;
  }

public:
  template <typename T, size_t... N>
  auto AddVarying(std::string name) {
    if (not IsValidIdentifier(name))
      throw InvalidIdentifierError{};

    auto tensors = std::make_unique<VectorOfTensors<T, N...>>();
    auto [it, inserted] = varying_.emplace(name_, std::move(tensors));
    auto *result = it->second.get();

    if (not inserted and
        (result->ComponentTypeID() != tensors->ComponentTypeID() or
         result->Shape() != tensors->Shape()))
      throw FieldExistsError{};

    result->Resize(Size());

    return VectorOfTensorsRef<T, N...>{
        *static_cast<VectorOfTensors<T, N...> *>(result)};
  }

  template <typename T, size_t ...N>
  auto AddUniform(std::string name) {
    // TODO auto AddUniform<T, N...>(string name)
  }

public:
  // TODO IntervalOf<size_t> Create(size_t count)

  // TODO void Erase(IntervalOf<size_t> indices)

  // TODO bool Dirty() const

  // TODO void Dirty(bool) const

public:
  void Resize(size_t new_size) {
    for (auto &[name, field] : varying_)
      field->Resize(new_size);

    size_ = new_size;
  }

  // TODO void Permute(SpanOf<size_t> perm)

private:
  template <typename S>
  static bool IsValidIdentifier(S const &s) {
    static std::regex const valid_ident{R"([a-zA-Z][a-zA-Z0-9_]*)"};
    using std::begin, std::end;
    return std::regex_match(begin(s), end(s), valid_ident);
  }

private:
  std::string name_;
  std::string type_;
  std::unordered_set<std::string> tags_;

  size_t size_;

  std::unordered_map<std::string, std::unique_ptr<Tensors>> uniform_;
  std::unordered_map<std::string, std::unique_ptr<Tensors>> varying_;
};

} // namespace prtcl

#endif // PRTCL_GROUP_HPP
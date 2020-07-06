#ifndef PRTCL_SRC_PRTCL_SCHEMES_SCHEME_BASE_HPP
#define PRTCL_SRC_PRTCL_SCHEMES_SCHEME_BASE_HPP

#include "../cxx/map.hpp"
#include "../data/model.hpp"
#include "../log.hpp"
#include "../util/neighborhood.hpp"

#include <functional>
#include <string>
#include <string_view>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl {

class SchemeBase {
public:
  using ProcedureFunction =
      std::function<void(SchemeBase &, Neighborhood const &)>;

public:
  virtual ~SchemeBase() = default;

public:
  virtual std::string GetFullName() const = 0;

public:
  virtual void Load(Model &model) = 0;

public:
  void RunProcedure(std::string_view name, Neighborhood const &nhood) {
    if (auto it = procedures_.find(name); it != procedures_.end())
      it->second(*this, nhood);
    else
      throw "procedure does not exist";
  }

public:
  auto GetProcedureNames() const {
    return boost::make_iterator_range(procedures_) | boost::adaptors::map_keys;
  }

public:
  virtual std::string_view GetPrtclSourceCode() const = 0;

protected:
  template <typename Impl>
  void RegisterProcedure(
      std::string_view name, void (Impl::*proc)(Neighborhood const &)) {
    // storing the procedure member functions like this ensures that the scheme
    // object itself can be copied since the object itself is _not_ bound in the
    // stored std::function's
    procedures_.emplace(
        name, [proc](SchemeBase &self, Neighborhood const &nhood) {
          // call the pointer to the procedure implementation
          (static_cast<Impl *>(&self)->*proc)(nhood);
        });
  }

private:
  cxx::het_flat_map<std::string, ProcedureFunction> procedures_;
};

class SchemeRegistry {
private:
  SchemeRegistry() {
    log::Debug("lib", "prtcl", "SchemeRegistry::SchemeRegistry()");
  }

public:
  SchemeRegistry(SchemeRegistry const &) = delete;
  SchemeRegistry &operator=(SchemeRegistry const &) = delete;
  SchemeRegistry(SchemeRegistry &&) = delete;
  SchemeRegistry &operator=(SchemeRegistry &&) = delete;

public:
  template <typename Scheme>
  bool RegisterScheme(std::string_view name) {
    log::Debug("lib", "prtcl", "registered scheme ", name);
    schemes_.emplace(name, [] { return new Scheme; });
    return true;
  }

public:
  std::unique_ptr<SchemeBase> NewScheme(std::string_view name) {
    if (auto it = schemes_.find(name); it != schemes_.end())
      return std::unique_ptr<SchemeBase>{it->second()};
    else
      throw "scheme is not registered";
  }

public:
  auto GetSchemeNames() const {
    return boost::make_iterator_range(schemes_) | boost::adaptors::map_keys;
  }

public:
  friend SchemeRegistry &GetSchemeRegistry();

private:
  cxx::het_flat_map<std::string, std::function<SchemeBase *()>> schemes_ = {};
};

SchemeRegistry &GetSchemeRegistry();

template <typename Scheme>
class SchemeRegistration {
public:
  SchemeRegistration(std::string_view name) {
    GetSchemeRegistry().RegisterScheme<Scheme>(name);
  }
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_SCHEMES_SCHEME_BASE_HPP

#include "module_math.hpp"

#include <prtcl/math.hpp>

namespace prtcl::lua {

sol::table ModuleMath(sol::state_view lua) {
  auto m = lua.create_table();

  using math::Index, math::Extent;
  using RSca = RealScalar;
  using RVec = RealVector;
  using RMat = RealMatrix;

  // Vector

  auto rvec = m.new_usertype<RVec>(
      "rvec", sol::factories(
                  [] { return RVec{}; }, [](Extent rows) { return RVec{rows}; },
                  [](sol::as_table_t<std::vector<double>> tbl) {
                    auto const &vec = tbl.value();
                    RVec result(vec.size());
                    for (Index i = 0; i < static_cast<Index>(vec.size()); ++i)
                      result[i] = vec[static_cast<size_t>(i)];
                    return result;
                  }));

  rvec.set_function(
      "resize", [](RVec &self, Extent rows) { self.resize(rows); });

  rvec.set_function(sol::meta_function::to_string, [](RVec const &lhs) {
    return math::ToString(lhs);
  });
  // vec @ vec
  rvec.set_function(
      sol::meta_function::addition,
      [](RVec const &lhs, RVec const &rhs) -> RVec { return lhs + rhs; });
  rvec.set_function(
      sol::meta_function::subtraction,
      [](RVec const &lhs, RVec const &rhs) -> RVec { return lhs - rhs; });
  rvec.set_function(
      sol::meta_function::multiplication,
      sol::overload(
          // sca * vec, vec * sca
          [](RSca const &lhs, RVec const &rhs) -> RVec { return lhs * rhs; },
          [](RVec const &lhs, RSca const &rhs) -> RVec { return lhs * rhs; }));
  rvec.set_function(
      sol::meta_function::division,
      [](RVec const &lhs, RSca const &rhs) -> RVec { return lhs / rhs; });

  rvec.set_function("norm", [](RVec const &vec) { return math::norm(vec); });

  rvec.set_function("cross", [](RVec const &a, RVec const &b) -> RVec {
    return math::cross(math::Tensor<double, 3>{a}, math::Tensor<double, 3>{b});
  });

  rvec.set_function(
      "zeros", [](Extent rows) -> RVec { return RVec::Zero(rows); });
  rvec.set_function(
      "ones", [](Extent rows) -> RVec { return RVec::Ones(rows); });
  rvec.set_function(
      "unit", sol::overload(
                  [](Extent rows) -> RVec { return RVec::Unit(rows, 0); },
                  [](Extent rows, Index index) -> RVec {
                    return RVec::Unit(rows, index);
                  }));

  // Matrix

  auto rmat = m.new_usertype<RMat>(
      "rmat",
      sol::factories(
          [] { return RMat{}; },
          [](size_t rows, size_t cols) {
            return RMat{static_cast<Extent>(rows), static_cast<Extent>(cols)};
          },
          [](sol::nested<std::vector<std::vector<double>>> tbl) {
            auto const &rows = tbl.value();
            auto const row_count = static_cast<Index>(rows.size());
            if (row_count == 0)
              throw "row count must be greater than zero";
            auto const col_count = static_cast<Index>(rows[0].size());
            if (col_count == 0)
              throw "column count must be greater than zero";
            RMat result(rows.size(), rows[0].size());
            for (Index row_i = 0; row_i < row_count; ++row_i) {
              auto const &row = rows[static_cast<size_t>(row_i)];
              if (col_count != static_cast<Index>(row.size()))
                throw "inconsistent column count";
              for (Index col_i = 0; col_i < col_count; ++col_i) {
                result(row_i, col_i) = row[static_cast<size_t>(col_i)];
              }
            }
            return result;
          }));

  rmat.set_function("resize", [](RMat &self, size_t rows, size_t cols) {
    self.resize(static_cast<Index>(rows), static_cast<Extent>(cols));
  });

  rmat.set_function(sol::meta_function::to_string, [](RMat const &lhs) {
    // TODO: thanks to Eigen this _will_ format N×1 matrices as vectors
    return math::ToString(lhs);
  });
  rmat.set_function(
      sol::meta_function::addition,
      [](RMat const &lhs, RMat const &rhs) -> RMat { return lhs + rhs; });
  rmat.set_function(
      sol::meta_function::subtraction,
      [](RMat const &lhs, RMat const &rhs) -> RMat { return lhs - rhs; });
  rmat.set_function(
      sol::meta_function::multiplication,
      sol::overload(
          // sca * mat, mat * sca
          [](RSca const &lhs, RMat const &rhs) -> RMat { return lhs * rhs; },
          [](RMat const &lhs, RSca const &rhs) -> RMat { return lhs * rhs; },
          // mat * vec
          [](RMat const &lhs, RVec const &rhs) -> RVec { return lhs * rhs; },
          // mat * mat
          [](RMat const &lhs, RMat const &rhs) -> RMat { return lhs * rhs; }));
  rmat.set_function(
      sol::meta_function::division,
      [](RMat const &lhs, RSca const &rhs) -> RMat { return lhs / rhs; });

  rmat.set_function("zeros", [](Extent rows, Extent cols) -> RMat {
    return RMat::Zero(rows, cols);
  });
  rmat.set_function("ones", [](Extent rows, Extent cols) -> RMat {
    return RMat::Ones(rows, cols);
  });
  rmat.set_function("identity", [](Extent rows, Extent cols) -> RMat {
    return RMat::Identity(rows, cols);
  });

  return m;
}

} // namespace prtcl::lua

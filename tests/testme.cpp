#include <prtcl/data/group.hpp>
#include <prtcl/data/vector_of_tensors.hpp>
#include <prtcl/log.hpp>
#include <prtcl/math.hpp>

int main() {
  namespace log = prtcl::log;
  namespace m = prtcl::math;

  log::Debug("app", "main", "testme debug");
  log::Info("app", "main", "testme info");
  log::Warning("app", "main", "testme warning");
  log::Error("app", "main", "testme error");

  m::Tensor<float> a = 3.0f;

  auto const u = a * m::ones<float, 1, 2>();
  auto const v = m::Constant<float, 1, 2>(2);

  log::Info(
      "app", "testme", "u = ", m::ToString(u), " v = ", m::ToString(v),
      " cmul(u, v) = ", m::ToString(m::cmul(u, v)));

  auto const I = m::identity<float, 3, 4>();
  log::Info("app", "testme", "I_{3,4} = ", m::ToString(I));

  prtcl::Group group{"f", "fluid"};
  group.Resize(1024);

  auto data = group.AddVarying<float, 1, 2>("f_1_2");

  log::Info("app", "testme", "data.Size() == ", data.Size());

  prtcl::AccessToVectorOfTensors<float, 1, 2> data_ref_a{data};
  prtcl::AccessToVectorOfTensors<float, 1, 2> data_ref_b{data};
  log::Info("app", "testme", "a == b ? ", data_ref_a == data_ref_b);
}

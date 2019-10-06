#include <algorithm>
#include <type_traits>

#include <prtcl/data/host/host_linear_data.hpp>
#include <prtcl/data/sycl/sycl_linear_access.hpp>

template <typename T, typename Data> void test_device_access(Data &data) {
  using namespace prtcl;

  {
    sycl::queue queue;

    sycl::buffer<T, 1> buf{data.size()};

    queue.submit([&](auto &cgh) {
      auto rw = buf.template get_access<sycl::access::mode::write>(cgh);

      cgh.template parallel_for<class testing>(
          buf.get_range(), [=](sycl::item<1> id) { rw[id] = rw[id]; });
    });
  }
}

int main() {
  using namespace prtcl;

  using T = double;

  host_linear_data<T> data;

  // resize the data
  data.resize(10);

  // test_host_access<T>(data);
  test_device_access<T>(data);
}

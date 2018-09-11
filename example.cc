#include <iostream>
#include <vector>

#include <upca/upca.h>

int main(int, char *[]) {
  using upca_backend = pmu;

  upca::resolver<upca_backend> pcs;

  try {
    pcs.add("cache-misses");
    pcs.add("l1d.repl");
  } catch (const std::runtime_error &e) {
    std::cout << e.what() << std::endl;
  }
  for (const auto &pc : pcs) {
    using namespace upca::arch;
    std::cout << pc.data() << std::endl;
  }

  std::vector<uint64_t> data(4);

  /* thread needs to be pinned before configure is called */

  auto roi = pcs.configure();
  roi->start(data);
  roi->stop(data);
}

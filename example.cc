#include "upca/upca.h"

int main(int argc, char *argv[]) {
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

  auto roi = pcs.configure();

  upca::resolver<upca::arch::aarch32::aarch32_pmu> arm;

  try {
    arm.add("cache-misses");
  } catch (const std::runtime_error &e) {
    std::cout << e.what() << std::endl;
  }
}

#include "upca/upca.h"

int main(int argc, char *argv[]) {
  using backend_type =
      upca::arch::arch_common_base<upca::arch::x86_64::x86_linux_mckernel<
          upca::arch::x86_64::mck_mckmsr, upca::arch::x86_64::linux_rawmsr>>;

  upca::resolver<backend_type> pcs;
  pcs.add("cache-misses");
  pcs.add("l1d.repl");

  for (const auto &pc : pcs) {
    using namespace upca::arch;
    std::cout << pc.data() << std::endl;
  }

  std::vector<uint64_t> data(4);

  pcs.configure(data);
}

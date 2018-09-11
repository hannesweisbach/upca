#include <iostream>
#include <sstream>
#include <vector>

#include <upca/upca.h>

int main(int argc, char *argv[]) {
  upca::resolver<pmu> pcs;

  try {
    pcs.add("cache-misses");
    pcs.add("l1d.repl");
  } catch (const std::runtime_error &e) {
    std::cout << e.what() << std::endl;
  }

  for (int i = 1; i < argc; ++i) {
    try {
      pcs.add(argv[i]);
    } catch (const std::runtime_error &e) {
      std::cout << "Error loading PMC '" << argv[i] << "': " << e.what()
                << std::endl;
    }
  }

  for (const auto &pc : pcs) {
    using namespace upca;
    std::cout << pc.data() << std::endl;
  }

  std::vector<uint64_t> data(pcs.size());

  /* thread needs to be pinned before configure is called */
  auto roi = pcs.configure();
  roi->start(data);
  {
    std::ostringstream os;
    os << "Hello World!" << std::endl;
  }
  roi->stop(data);

  std::cout << "cycles:\t" << data.at(0) << std::endl;
  for (unsigned i = 0; i < pcs.size() - 1; ++i) {
    std::cout << pcs.at(i).name() << ":\t" << data.at(i + 1) << std::endl;
  }
}

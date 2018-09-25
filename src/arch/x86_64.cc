
#include <linux/perf_event.h>

#include <upca/upca.h>

namespace upca {
namespace arch {
namespace x86_64 {

#if defined(JEVENTS_FOUND)
resolver::resolver() {
  /*
   * Initialize the racy libjevents, see:
   * https://github.com/andikleen/pmu-tools/tree/master/jevents#initializationmultithreading
   */

  read_events(nullptr);
}

struct perf_event_attr resolver::resolve(const std::string &name) const {
  struct perf_event_attr attr;
  const int err = resolve_event(name.c_str(), &attr);
  if (err) {
    using namespace std::string_literals;
    throw std::runtime_error("Error resolving event \""s + name + "\"\n");
  }
  return attr;
}
#endif

x86_pmc_base::~x86_pmc_base() = default;

} // namespace x86_64
} // namespace arch

std::ostream &operator<<(std::ostream &os, const struct perf_event_attr &c) {
  os << std::hex << c.type << " " << std::dec << " " << c.size << std::hex
     << " " << c.config << std::dec;
  return os;
}

} // namespace upca


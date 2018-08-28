#pragma once

#include <stdexcept>
#include <string>
#include <iostream>

#include <linux/perf_event.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <jevents.h>

#ifdef __cplusplus
}
#endif

namespace upca {
namespace arch {

class jevents_resolver {
  /*
   * Initialize the racy libjevents, see:
   * https://github.com/andikleen/pmu-tools/tree/master/jevents#initializationmultithreading
   */
  struct jevent_initializer {
    jevent_initializer() { read_events(NULL); }
  };
  static struct jevent_initializer init_;

public:
  using config_type = struct perf_event_attr;

  static struct perf_event_attr resolve(const std::string &name) {
    struct perf_event_attr attr;
    const int err = resolve_event(name.c_str(), &attr);
    if (err) {
      using namespace std::string_literals;
      throw std::runtime_error("Error resolving event \""s + name + "\"\n");
    }
    return attr;
  }

};

inline std::ostream &operator<<(std::ostream &os,
                                const struct perf_event_attr &c) {
  os << std::hex << c.type << " " << std::dec << " " << c.size << std::hex
     << " " << c.config << std::dec;
  return os;
}
} // namespace arch
} // namespace upca


#include <upca/upca.h>

namespace upca {
namespace arch {
namespace bgq {

#if defined(HAVE_BGPM)
resolver::config_type resolver::resolve(const std::string &name) {
  /* Blah, inefficient, but whatevs … init costs don't matter, right? */
  for (const auto &event : events) {
    for (int event_id = 0; event_id < PEVT_LAST_EVENT; ++event_id) {
      auto label = Bgpm_GetEventIdLabel(event_id);
      if (strcasecmp(name.c_str(), label) == 0) {
        return event_id;
      }
    }
  }

  throw std::runtime_error("Event \"" + name + "\" not found.");
}
#endif

} // namespace bgq
} // namespace arch
} // namespace upca

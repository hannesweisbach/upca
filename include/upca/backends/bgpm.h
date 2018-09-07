#pragma once

#include <array>
#include <cstring>

#if defined(HAVE_BGPM)
#include <bgpm/include/bgpm.h>

namespace upca {
namespace platform {

/*
 * See bgq-driver/spi/include/upci/events.h for a list of events.
 * Link for your convenience:
 * https://github.com/jedbrown/bgq-driver/blob/master/spi/include/upci/events.h
 */
class bgpm_resolver {
public:
  using config_type = unsigned;

  static config_type resolve(const std::string &name) {
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
};
} // namespace platform
} // namespace upca

#endif

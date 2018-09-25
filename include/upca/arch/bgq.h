#pragma once

#include <hwi/include/bqc/A2_inlines.h>

#if defined(HAVE_BGPM)
#include <bgpm/include/bgpm.h>
#endif

#include <stdexcept>
#include <string>

#include <cstring>

namespace upca {
namespace arch {
namespace bgq {

struct Reason {
  static constexpr const char reason[] =
      "PMU support not implemented for BG/Q.";
};

struct timestamp_t {
  static inline uint64_t timestamp() { return GetTimeBase(); }
};

#if !defined(HAVE_BGPM)

using pmu = detail::basic_pmu<timestamp_t, Reason>;

#else

/*
 * By default, the Bgpm interface prints a message and exits the
 * user's program if a usage error occurs - we'll let that happen
 * so no error checking needed.
 * Source: https://github.com/jedbrown/bgq-driver
 * Bqpm_PrintOnError(), Bgpm_ExitOnError() -> default: yes
 */

class resolver {
public:
  static unsigned resolve(const std::string &name);
};

class pmu {
  unsigned event_set;

public:
  using resolver_type = resolver;

  template <typename T>
  pmu(const T &pmcs)
      : event_set((static_cast<void>(Bgpm_Init(BGPM_MODE_SWDISTRIB)),
                   static_cast<unsigned>(Bgpm_Init(BGPM_MODE_SWDISTRIB)))) {
    for (const auto &pmc : pmcs) {
      Bgpm_AddEvent(event_set, pmc.data());
    }

    if (!pmcs.empty()) {
      /* Using an empty event set is an error */
      Bgpm_Apply(event_set);
    }
  }

  ~pmu() {
    Bgpm_DeleteEventSet(event_set);
    Bgpm_Disable();
  }

  pmu(const pmu &) = delete;
  pmu(pmu &&) = delete;

  uint64_t timestamp_begin() { return timestamp_t::timestamp(); }
  uint64_t timestamp_end() { return timestamp_t::timestamp(); }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) {
    const auto num_events = Bgpm_NumEvents(event_set);

    if (num_events > 0) {
      Bgpm_Reset(event_set);
      Bgpm_Start(event_set);
    }

    return num_events;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> o) {
    const auto num_events = Bgpm_NumEvents(event_set);

    if (num_events > 0) {
      Bgpm_Stop(event_set);
    }

    for (int i = 0; i < num_events; ++i) {
      Bgpm_ReadEvent(event_set, static_cast<unsigned>(i), &o[i]);
    }

    return 0;
  }
};

#endif

} // namespace bgq
} // namespace arch
} // namespace upca


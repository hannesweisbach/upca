#pragma once

//#include <hwi/include/bqc/A2_inlines.h>
#if defined(HAVE_BGPM)
#include <bgpm/include/bgpm.h>
#endif

#include <stdexcept>

#include <cstring>

namespace upca {
namespace arch {
namespace bgq {

namespace {
static constexpr const char reason[] = "PMU support not implemented for BG/Q.";

struct timestamp_t {
  static inline uint64_t timestamp() { return GetTimeBase(); }
};
} // namespace

#if !defined(HAVE_BGPM)

using pmu = detail::basic_pmu<timestamp_t, reason>;

#else

/*
 * By default, the Bgpm interface prints a message and exits the
 * user's program if a usage error occurs - we'll let that happen
 * so no error checking needed.
 * Source: https://github.com/jedbrown/bgq-driver
 * Bqpm_PrintOnError(), Bgpm_ExitOnError() -> default: yes
 */

class pmu {
  int event_set = -1;

public:
  using resolver_type = upca::platform::bgpm_resolver;

  template <typename T>
  pmu(const T &pmcs)
      : event_set((Bgpm_Init(BGPM_MODE_SWDISTRIB), Bgpm_CreateEventSet())) {
    for (const auto &pmc : pmcs) {
      Bgpm_AddEvent(event_set, pmc.data());
    }

    Bgpm_Apply(event_set);
  }

  ~pmu() {
    Bgpm_DeleteEventSet(event_set);
    Bgpm_Disable();
  }

  uint64_t timestamp_begin() { return timestamp_t::timestamp(); }
  uint64_t timestamp_end() { return timestamp_t::timestamp(); }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) {
    const auto num_events = Bqpm_NumEvents(event_set);

    Bgpm_Reset(event_set);
    Bgpm_Start(event_set);

    return num_events;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> o) {
    Bgpm_Stop(event_set);

    const auto num_events = Bgpm_NumEvents(event_set);

    for (int i = 0; i < num_events; ++i) {
      Bgpm_ReadEvent(event_set, i, &o[i]);
    }

    return 0;
  }
};

#endif

} // namespace bgq
} // namespace arch
} // namespace upca


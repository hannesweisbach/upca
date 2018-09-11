#include <array>
#include <stdexcept>

#include <cstring>

#include <upca/upca.h>

namespace upca {
namespace arch {
namespace aarch64 {

static bool is_pmu_enabled() {
  enum PMUSERENR_EL0 : uint64_t {
    EN = 0x1,
    SW = 0x2,
    CR = 0x4,
    ER = 0x8,
  };

  uint64_t value;
  __asm__ volatile("mrs %0, PMUSERENR_EL0" : "=r"(value));

  if (!(value & (PMUSERENR_EL0::EN | PMUSERENR_EL0::CR))) {
    throw std::runtime_error(
        "EL0 access to PMU disabled: cycle counter not readable.");
  }

  /* PMUSERENR_EL=::ER is not sufficient. */
  return value & (PMUSERENR_EL0::EN);
}

static unsigned get_num_counters() {
  uint64_t value;
  __asm__ volatile("mrs %0, PMCR_EL0" : "=r"(value));

  if (!(value & 1)) {
    throw std::runtime_error("PMU not enabled");
  }

  const unsigned num_counters = (value >> 11) & 0x1f;

  return num_counters;
}

resolver::resolver()
    : pmu_enabled_(is_pmu_enabled()),
      max_counters_(pmu_enabled_ ? get_num_counters() : 0) {}

uint64_t resolver::resolve(const std::string &name) const {
  if (!pmu_enabled_) {
    throw std::runtime_error("PMU not enabled.");
  }

  /* ARM ARM D5.10 */
  struct pmu_event {
    const char *name;
    uint64_t type;
  };
  static const struct pmu_event events[] = {
      {"SW_INCR", 0x00},          {"L1I_CACHE_REFILL", 0x01},
      {"L1D_CACHE_REFILL", 0x03}, {"L1D_CACHE", 0x04},
      {"MEM_ACCESS", 0x13},       {"L2D_CACHE", 0x16},
      {"L2D_CACHE_REFILL", 0x17}, {"CHAIN", 0x1e},
  };

  for (const auto &event : events) {
    if (strcasecmp(event.name, name.c_str()) == 0) {
      return event.type;
    }
  }

  throw std::runtime_error("PMC " + name + " not found.");
}

} // namespace aarch64
} // namespace arch
} // namespace upca

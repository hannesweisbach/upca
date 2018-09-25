#pragma once

namespace upca {
namespace arch {
namespace aarch32 {

static constexpr const char reason[] =
    "PMU support not implemented for AArch32.";

struct timestamp_t {
  static inline uint64_t timestamp() {
    uint32_t value;
    // Read CCNT Register
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(value));
    return value;
  }
};

using pmu = detail::basic_pmu<timestamp_t, reason>;

} // namespace aarch32
} // namespace arch
} // namespace upca

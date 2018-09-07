#pragma once

namespace upca {
namespace arch {
namespace aarch32 {

namespace {
static constexpr const char reason[] =
    "PMU support not implemented for AArch32.";

struct aarch32_timestamp {
  static inline uint64_t timestamp() {
    uint32_t value;
    // Read CCNT Register
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(value));
    return value;
  }
};
} // namespace

using aarch32_pmu = detail::basic_pmu<aarch32_timestamp, reason>;

} // namespace aarch32
} // namespace arch
} // namespace upca

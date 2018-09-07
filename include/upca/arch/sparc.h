#pragma once

namespace upca {
namespace arch {
namespace sparc {

namespace {
static constexpr const char reason[] = "PMU support not implemented for SPARC.";

struct sparc_timestamp {
  static inline uint64_t timestamp() {
    uint64_t value;
    __asm__ volatile("rd %%tick, %0\n" : "=r"(value));
    return value;
  }
};
} // namespace

using sparc_pmu = detail::basic_pmu<sparc_timestamp, reason>;

} // namespace sparc
} // namespace arch
} // namespace upca


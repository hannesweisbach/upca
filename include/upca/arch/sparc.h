#pragma once

namespace upca {
namespace arch {
namespace sparc {

struct Reason {
  static constexpr const char reason[] =
      "PMU support not implemented for SPARC.";
};

struct timestamp_t {
  static inline uint64_t timestamp() {
    uint64_t value;
    __asm__ volatile("rd %%tick, %0\n" : "=r"(value));
    return value;
  }
};

using pmu = detail::basic_pmu<timestamp_t, Reason>;

} // namespace sparc
} // namespace arch
} // namespace upca


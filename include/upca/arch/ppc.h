#pragma once

namespace upca {
namespace arch {
namespace ppc {

namespace {
static constexpr const char reason[] = "PMU support not implemented for PPC.";

struct timestamp_t {

  static inline uint64_t timestamp() {
#if defined(__powerpc64__) || defined(_ARCH_PPC64)
    uint64_t ticks;
    __asm__ volatile("mftb %0" : "=r"(ticks));
    return ticks;
#else
    unsigned int tbl, tbu0, tbu1;
    do {
      __asm__ volatile("mftbu %0" : "=r"(tbu0));
      __asm__ volatile("mftb %0" : "=r"(tbl));
      __asm__ volatile("mftbu %0" : "=r"(tbu1));
    } while (tbu0 != tbu1);
    return (static_cast<uint64_t>(tbu0) << 32) | tbl;
#endif
  }
};
} // namespace

using pmu = detail::basic_pmu<timestamp_t, reason>;

} // namespace ppc
} // namespace arch
} // namespace upca


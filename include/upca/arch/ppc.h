#pragma once

namespace upca {
namespace arch {
namespace ppc {

namespace {
static constexpr const char reason[] = "PMU support not implemented for PPC.";
}

class ppc_pmu {
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

public:
  using resolver_type = upca::arch::detail::null_resolver<reason>;
  template <typename T> ppc_pmu(const T &) {}

  uint64_t timestamp_begin() { return timestamp(); }
  uint64_t timestamp_end() { return timestamp(); }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) { return 0; }
  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t>) { return 0; }
};

} // namespace ppc
} // namespace arch
} // namespace upca


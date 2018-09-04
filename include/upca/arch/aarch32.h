#pragma once

namespace upca {
namespace arch {
namespace aarch32 {

namespace {
static constexpr const char reason[] =
    "PMU support not implemented for AArch32.";
}

class aarch32_pmu {
  static inline uint64_t timestamp() {
    uint32_t value;
    // Read CCNT Register
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(value));
    return value;
  }

public:
  using resolver_type = upca::arch::detail::null_resolver<reason>;
  template <typename T> aarch32_pmu(const T &) {}

  uint64_t timestamp_begin() { return timestamp(); }
  uint64_t timestamp_end() { return timestamp(); }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) { return 0; }
  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t>) { return 0; }
};

} // namespace aarch32
} // namespace arch
} // namespace upca

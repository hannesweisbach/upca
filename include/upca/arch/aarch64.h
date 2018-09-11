#pragma once

namespace upca {
namespace arch {
namespace aarch64 {

class resolver {
  const bool pmu_enabled_;
  const unsigned max_counters_;
public:
  using config_type = uint64_t;
  resolver();

  bool pmu_enabled() const { return pmu_enabled_; }
  unsigned max_counters() const { return max_counters_; }
  uint64_t resolve(const std::string &name);
};

class pmu {
  int counters = 0;

  static void dmb() { __asm__ volatile("dmb sy" ::: "memory"); }
  static void isb() { __asm__ volatile("isb"); }

  static inline uint64_t timestamp() {
    uint64_t value;
    // Read CCNT Register
    __asm__ volatile("mrs %0, cntvct_EL0\t\n" : "=r"(value));
    return value;
  }

  static void select_reg(const uint64_t reg) {
    __asm__ volatile("msr PMSELR_EL0, %0" : : "r"(reg));
  }

  static void enable_reg(const uint32_t reg) {
    __asm__ volatile("msr PMCNTENSET_EL0, %0" : : "r"(static_cast<uint64_t>(1) << reg));
  }

  static void disable_reg(const uint32_t reg) {
    __asm__ volatile("msr PMCNTENCLR_EL0, %0" : : "r"(static_cast<uint64_t>(1) << reg));
  }

  static uint64_t read_current_counter() {
    uint64_t value;
    __asm__ volatile("mrs %0, PMXEVCNTR_EL0" : "=r"(value));
    return value;
  }

  static void write_type(const uint64_t type) {
    uint64_t result = 0x08000000 | (type & 0xffff);
    __asm__ volatile("msr PMXEVTYPER_EL0, %0" : : "r"(result));
  }

public:
  using resolver_type = resolver;

  template <typename T> pmu(const T &pmcs) {
    uint64_t value;

    for (const auto &pmc : pmcs) {
      select_reg(counters);
      write_type(pmc.data());
      ++counters;
    }

    for (int i = counters; i > 0; --i) {
      enable_reg((unsigned)(i - 1));
      isb();
    }
  }

  ~pmu() {
    for (int i = 0; i < counters; ++i) {
      select_reg(i);
      disable_reg(i);
    }
  }

  uint64_t timestamp_begin() { return timestamp(); }
  uint64_t timestamp_end() { return timestamp(); }
  
  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t> ) {
    for (int i = 0; i < counters; ++i) {
      // clear overflow flag
      __asm__ volatile("msr PMOVSCLR_EL0, %0" : : "r"(static_cast<uint64_t>(1) << i));
      select_reg(i);
      // set counter register to 0
      __asm__ volatile("msr PMXEVCNTR_EL0, %0" : : "r"(static_cast<uint64_t>(0)));
    }

    isb();
    return counters;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> o) {
    uint64_t ovf = 0;
    __asm__ volatile("mrs %0, PMOVSSET_EL0" : "=r"(ovf));

    for (int i = 0; i < counters; ++i) {
      select_reg(i);
      o[i] = read_current_counter();
      if (ovf & (1 << i)) {
        printf("Warning, overflow in counter %d\n", i);
      }
    }

    return counters;
  }
};

} // namespace aarch64
} // namespace arch
} // namespace upca

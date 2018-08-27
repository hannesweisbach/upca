#pragma once

#define UPCA_ARCH_H

#include <memory>
#include <vector>

#include <stdint.h>

#include <gsl/gsl>

namespace upca {
namespace arch {

template <typename ARCH> class arch_common_base final {
private:
  ptrdiff_t offset_ = 0;
  const ptrdiff_t slice_;
  gsl::span<uint64_t> span_;
  ARCH arch_;

  auto current_pmu_span() { return span_.subspan(offset_ + 1, slice_ - 1); }
  auto &&current_time() { return span_.subspan(offset_, 1)[0]; }

  template <typename T> friend class resolver;

public:
  using resolver_type = typename ARCH::resolver_type;

  template <typename C>
  arch_common_base(const C &pmcs, gsl::span<uint64_t> output)
      : slice_(pmcs.size()), span_(output), arch_(pmcs) {}

  void start() {
    auto pmu_span = current_pmu_span();
    arch_.start(pmu_span.begin());
    current_time() = arch_.timestamp_begin();
  }

  void stop() {
    current_time() = arch_.timestamp_end() - current_time();
    auto pmu_span = current_pmu_span();
    arch_.stop(pmu_span.begin());
    offset_ += slice_;
  }
};

} // namespace arch
} // namespace upca

#ifdef __aarch64__
#  include "aarch64.h"
#elif defined(__sparc)
#  include "sparc.h"
#elif defined(__x86_64__)
#  include "x86_64.h"

/* MCK can be either:
 * - mck_rawmsr for raw MSR access via rdmsr/wrmsr mcK syscalls
 * - mck_mckmsr for PMU programmed via mcK pmc_* syscalls
 *
 * LINUX can be any of:
 * - linux_perf for using perf
 * - linux_jevents for using jevents
 * - linux_rawmsr for raw MSR access using /dev/cpu/N/msr
 */

using pmu = upca::arch::arch_common_base<upca::arch::x86_64::x86_linux_mckernel<
    upca::arch::x86_64::mck_mckmsr, upca::arch::x86_64::linux_rawmsr>>;

#elif defined(__bgq__)
#  include "bgq.h"
#elif defined(__ppc__) || defined(_ARCH_PPC) || defined(__PPC__)
#  include "ppc.h"
#else

#error "Unkown/Unsupported architecture"

#endif



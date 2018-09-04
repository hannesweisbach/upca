#pragma once

#define UPCA_ARCH_H

#include <memory>
#include <vector>

#include <stdint.h>

#include <gsl/gsl>

namespace upca {
namespace arch {

namespace detail {
template <const char *Reason> struct null_resolver {
  struct null_type {
    friend std::ostream &operator<<(std::ostream &os, const null_type &) {
      return os;
    }
  };
  using config_type = null_type;
  static null_type resolve(const std::string &) {
    throw std::runtime_error(Reason);
  }
};
} // namespace detail

template <typename ARCH> class arch_common_base final {
private:
  ptrdiff_t offset_ = 0;
  const ptrdiff_t slice_;
  ARCH arch_;

  template <typename T> friend class resolver;

public:
  using resolver_type = typename ARCH::resolver_type;

  template <typename C>
  arch_common_base(const C &pmcs, const unsigned external_pmcs = 0)
      : slice_(gsl::narrow<ptrdiff_t>(pmcs.size() + external_pmcs)), arch_(pmcs) {}

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t> output) {
    const auto count = arch_.start(output.subspan(1, slice_ - 1));
    output[0] = arch_.timestamp_begin();
    return count + 1;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> output) {
    output[0] = arch_.timestamp_end() - output[0];
    const auto count = arch_.stop(output.subspan(1, slice_ - 1));
    return count + 1;
  }
};

} // namespace arch
} // namespace upca

#ifdef __aarch64__
#  include "aarch64.h"
#elif defined(__ARM_ARCH_7A__)
#  include "aarch32.h"
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

#ifdef JEVENTS_FOUND
using pmu = upca::arch::arch_common_base<upca::arch::x86_64::x86_linux_mckernel<
    upca::arch::x86_64::mck_mckmsr, upca::arch::x86_64::linux_rawmsr>>;
#else
using pmu = upca::arch::arch_common_base<upca::arch::x86_64::x86_64_pmu>;
#endif

#elif defined(__bgq__)
#  include "bgq.h"
#elif defined(__ppc__) || defined(_ARCH_PPC) || defined(__PPC__)
#  include "ppc.h"
#else

#error "Unkown/Unsupported architecture"

#endif



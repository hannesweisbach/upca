#pragma once

#ifndef UPCA_ARCH_H
#error "Don't include this file directly. Use #include <upca/upca.h>."
#endif

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <gsl/gsl>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef JEVENTS_FOUND

#include <inttypes.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <jevents.h>
#include <rdpmc.h>
#ifdef __cplusplus
}
#endif

#include <linux/perf_event.h>
#include <sys/ioctl.h>

#endif

namespace upca {
namespace arch {
namespace x86_64 {

class x86_64_base_pmu {
public:
  uint64_t timestamp_begin() {
    unsigned high, low;
    __asm__ volatile("CPUID\n\t"
                     "RDTSC\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\t"
                     : "=r"(high), "=r"(low)::"%rax", "%rbx", "%rcx", "%rdx");

    return static_cast<uint64_t>(high) << 32 | low;
  }

  uint64_t timestamp_end() {
    unsigned high, low;
    __asm__ volatile("RDTSCP\n\t"
                     "mov %%edx,%0\n\t"
                     "mov %%eax,%1\n\t"
                     "CPUID\n\t"
                     : "=r"(high), "=r"(low)::"%rax", "%rbx", "%rcx", "%rdx");
    return static_cast<uint64_t>(high) << 32 | low;
  }
};

#ifdef JEVENTS_FOUND

#ifndef __APPLE__

/*
 * SYSCALL_HANDLED(601, pmc_init) int counter, int type, int mode
 * SYSCALL_HANDLED(602, pmc_start)
 * SYSCALL_HANDLED(603, pmc_stop)
 * SYSCALL_HANDLED(604, pmc_reset)
 *
 * Yes, the interface is that fucked up. counter is either int or unsigned long,
 * depending on the syscall …
 */

static inline long mck_pmc_init(int counter, int type, unsigned mode) {
  return syscall(601, counter, type, mode);
}

static inline long mck_pmc_start(unsigned long counter) {
  return syscall(602, counter);
}
static inline long mck_pmc_stop(unsigned long counter) { return syscall(603, counter); }
static inline long mck_pmc_reset(int counter) { return syscall(604, counter); }

static inline int mck_is_mckernel() { return syscall(732) == 0; }

#else

static inline int mck_is_mckernel() { return 0; }

#endif

class fd {
  int fd_;

public:
  explicit fd(int fd) : fd_(fd) {}
  fd(std::nullptr_t = nullptr) : fd_(-1) {}
  ~fd() {
    if (fd_ != -1) {
      close(fd_);
    }
  }
  fd(fd &&l) {
    fd_ = l.fd_;
    l.fd_ = -1;
  }
  explicit operator bool() { return fd_ != -1; }
  explicit operator int() { return fd_; }
  friend bool operator==(const fd &l, const fd &r) { return l.fd_ == r.fd_; }
  friend bool operator!=(const fd &l, const fd &r) { return !(l == r); }
};

static inline std::unique_ptr<fd> make_perf_fd(struct perf_event_attr &attr) {
  const int fd = perf_event_open(&attr, 0, -1, -1, 0);

  if (fd < 0) {
    std::ostringstream os;
    os << "perf_event_open() failed: " << errno << strerror(errno) << std::endl;
    throw std::runtime_error(os.str());
  }
  return std::make_unique<class fd>(fd);
}

static uint64_t rdpmc(const uint32_t counter) {
  uint32_t low, high;
  __asm__ volatile("rdpmc" : "=a"(low), "=d"(high) : "c"(counter));
  return static_cast<uint64_t>(high) << 32 | low;
}

enum msrs {
  IA32_PMC_BASE = 0x0c1,
  IA32_PERFEVTSEL_BASE = 0x186,
  IA32_FIXED_CTR_CTRL = 0x38d,
  IA32_PERF_GLOBAL_STATUS = 0x38e,
  IA32_PERF_GLOBAL_CTRL = 0x38f,
  IA32_PERF_GLOBAL_OVF_CTRL = 0x390,
  IA32_PERF_CAPABILITIES = 0x345,
  IA32_DEBUGCTL = 0x1d9,
};

static inline void cpuid(const int code, uint32_t *a, uint32_t *b, uint32_t *c,
                         uint32_t *d) {
  asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code));
}

#define MASK(v, high, low) ((v >> low) & ((1 << (high - low + 1)) - 1))

template <typename POLICY> static void pmu_info(const POLICY &msr) {
  uint32_t eax, ebx, ecx, edx;

  cpuid(0x1, &eax, &ebx, &ecx, &edx);

  std::cout << std::hex;

  std::cout << "EAX: " << eax << std::endl;
  std::cout << "EBX: " << ebx << std::endl;
  std::cout << "ECX: " << ecx << std::endl;
  std::cout << "EDX: " << edx << std::endl;

  if (ecx & (1 << 15)) {
    const uint64_t caps = msr.rdmsr(IA32_PERF_CAPABILITIES);
    const int vmm_freeze = MASK(caps, 12, 12);
    std::cout << "Caps: " << caps << std::endl;
    std::cout << "VMM Freeze: " << std::boolalpha
              << static_cast<bool>(vmm_freeze) << std::noboolalpha << std::endl;
    if (vmm_freeze) {
      const uint64_t debugctl = msr.rdmsr(IA32_DEBUGCTL);
      msr.wrmsr(IA32_DEBUGCTL, debugctl & ~(1 << 14));
    }
  }

  cpuid(0xa, &eax, &ebx, &ecx, &edx);

  const unsigned version = MASK(eax, 7, 0);
  const unsigned counters = MASK(eax, 15, 8);
  const unsigned width = MASK(eax, 23, 16);

  const unsigned ffpc = MASK(edx, 4, 0);
  const unsigned ff_width = MASK(edx, 12, 5);

  std::cout << "EAX: " << eax << std::endl;
  std::cout << "EBX: " << ebx << std::endl;
  std::cout << "ECX: " << ecx << std::endl;
  std::cout << "EDX: " << edx << std::endl;

  std::cout << std::dec;

  std::cout << "PMC version " << version << std::endl;
  std::cout << "PMC counters: " << counters << std::endl;
  std::cout << "PMC width: " << width << std::endl;

  std::cout << "FFPCs: " << ffpc << ", width: " << ff_width << std::endl;
}

struct x86_pmc_base {
  virtual ~x86_pmc_base() = default;
  virtual gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) = 0;
  virtual gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t>) = 0;
};

struct mck_mck_policy {
  static long init(const int active, const uint64_t config) {
    return mck_pmc_init(active, gsl::narrow<int>(config), 0x4);
  }

  /* writes MSR_IA32_PMC0 + i to 0 */
  static void reset(const int i) { mck_pmc_reset(i); }
  /* sets and clears bits in MSR_PERF_GLOBAL_CTRL */
  static void start(const unsigned long mask) {
    const auto err = mck_pmc_start(mask);
    if (err) {
      std::cerr << "Error starting PMCs" << std::endl;
    }
  }
  static void stop(const unsigned long mask) {
    const auto err = mck_pmc_stop(mask);
    if (err) {
      std::cerr << "Error stopping PMCs" << std::endl;
    }
  }
  static uint64_t read(const unsigned i) { return rdpmc(i); }
};

class msr_mck {
public:
  msr_mck() {}
  uint64_t rdmsr(const uint32_t reg) const {
    return static_cast<unsigned long>(syscall(850, reg));
  }
  auto wrmsr(const uint32_t reg, const uint64_t val) const {
    /* always returns 0 */
    return syscall(851, reg, val);
  }
};

class msr_linux {
  std::unique_ptr<fd> fd_;

  static unsigned cpu() {
    int cpu = sched_getcpu();
    if (cpu < 0) {
      using namespace std::string_literals;
      throw std::runtime_error("Error getting CPU number: "s + strerror(errno) +
                               " (" + std::to_string(errno) + ")\n");
    }
    /* TODO: check thread affinity mask
     * - has a single CPU
     * - this single CPU is this CPU
     */
    return static_cast<unsigned>(cpu);
  }

public:
  msr_linux()
      : fd_(std::make_unique<fd>(open(
            ("/dev/cpu/" + std::to_string(cpu()) + "/msr").c_str(), O_RDWR))) {}

  uint64_t rdmsr(const uint32_t reg) const {
    uint64_t v;
    const ssize_t ret = pread(static_cast<int>(*fd_), &v, sizeof(v), reg);
    if (ret != sizeof(v)) {
      std::cerr << "Reading MSR " << std::hex << reg << std::dec << " failed."
                << std::endl;
    }
    return v;
  }

  auto wrmsr(const uint32_t reg, const uint64_t val) const {
    const ssize_t ret = pwrite(static_cast<int>(*fd_), &val, sizeof(val), reg);
    if (ret != sizeof(val)) {
      std::cerr << "Writing MSR " << std::hex << reg << std::dec << " failed."
                << std::endl;
    }
    return ret;
  }
};

template <typename MSR> class rawmsr_policy : MSR {
  uint64_t global_ctrl_;

public:
  rawmsr_policy() : global_ctrl_(this->rdmsr(IA32_PERF_GLOBAL_CTRL)) {
    // pmu_info(static_cast<MSR&>(*this));
  }
  ~rawmsr_policy() { this->wrmsr(IA32_PERF_GLOBAL_CTRL, global_ctrl_); }
  int init(const int i, const uint64_t config) {
    const uint64_t v = (1 << 22) | (1 << 16) | config;
    this->wrmsr(IA32_PERFEVTSEL_BASE + static_cast<uint32_t>(i), v);
    return 0;
  }

  void reset(const int i) {
    this->wrmsr(IA32_PMC_BASE + static_cast<uint32_t>(i), 0);
  }
  void start(const unsigned long mask) {
    this->wrmsr(IA32_PERF_GLOBAL_OVF_CTRL, 0);
    this->wrmsr(IA32_PERF_GLOBAL_CTRL, mask);
  }
  void stop(const unsigned long /* mask */) {
    this->wrmsr(IA32_PERF_GLOBAL_CTRL, 0);
    const uint64_t ovf = this->rdmsr(IA32_PERF_GLOBAL_STATUS);
    if (ovf) {
      std::cout << "Overflow: " << std::hex << ovf << std::dec << std::endl;
      this->wrmsr(IA32_PERF_GLOBAL_OVF_CTRL, 0);
    }
  }
  /* Should be equivalent to rdpmc instruction */
  uint64_t read(const unsigned i) { return this->rdmsr(IA32_PMC_BASE + i); }
};

template <typename ACCESS> class msr_pmc final : ACCESS, public x86_pmc_base {
  unsigned long active_mask_ = 0;
  int active = 0;

public:
  using resolver_type = upca::arch::jevents_resolver;

  template <typename T> msr_pmc(const T &pmcs) {
    for (const auto &pmc : pmcs) {
      const auto err = ACCESS::init(active, pmc.data().config);
      if (err) {
        std::cerr << "Error configuring PMU " << pmc.name() << " with "
                  << std::hex << pmc.data().config << std::dec << std::endl;
        continue;
      }
      ++active;
    }

    active_mask_ = static_cast<unsigned long>((1 << active) - 1);
  }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) override {
    for (int i = 0; i < active; ++i) {
      ACCESS::reset(i);
    }
    ACCESS::start(active_mask_);

    return active;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> buf) override {
    ACCESS::stop(active_mask_);
    for (int i = 0; i < active; ++i) {
      buf[i] = ACCESS::read(static_cast<uint32_t>(i));
    }
    return active;
  }
};

using mck_rawmsr = msr_pmc<rawmsr_policy<msr_mck>>;
using mck_mckmsr = msr_pmc<mck_mck_policy>;
using linux_rawmsr = msr_pmc<rawmsr_policy<msr_linux>>;

class linux_jevents final : public x86_pmc_base {
  struct rdpmc_t {
    struct rdpmc_ctx ctx;
    int close = 0;

    rdpmc_t(struct perf_event_attr attr) {
      const int err = rdpmc_open_attr(&attr, &ctx, nullptr);
      if (err) {
        throw std::runtime_error(std::to_string(errno) + ' ' + strerror(errno));
      }

      close = 1;
    }
    ~rdpmc_t() {
      if (close) {
        rdpmc_close(&ctx);
      }
    }
    rdpmc_t(const rdpmc_t &) = delete;
    rdpmc_t(rdpmc_t &&rhs) {
      this->ctx = rhs.ctx;
      this->close = rhs.close;
      rhs.close = 0;
    }

    uint64_t read() { return rdpmc_read(&ctx); }
  };

  std::vector<rdpmc_t> rdpmc_ctxs;

public:
  using resolver_type = upca::arch::jevents_resolver;

  template <typename T> linux_jevents(const T &pmcs) {
    for (const auto &pmc : pmcs) {
      rdpmc_ctxs.emplace_back(pmc.data());
    }
  }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t> buf) override {
    int idx = 0;
    for (auto &&pmc : rdpmc_ctxs) {
      buf[idx] = pmc.read();
      ++idx;
    }
    return idx;
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> buf) override {
    int idx = 0;
    for (auto &&pmc : rdpmc_ctxs) {
      buf[idx] = pmc.read() - buf[idx];
      ++idx;
    }
    return idx;
  }
};

class linux_perf final : public x86_pmc_base {
  std::vector<std::unique_ptr<fd>> perf_fds;

public:
  using resolver_type = upca::arch::jevents_resolver;

  template <typename T> linux_perf(const T &pmcs) {
    for (const auto &pmc : pmcs) {
      struct perf_event_attr pe;
      memset(&pe, 0, sizeof(pe));
      const auto &attr = pmc.data();
      pe.config = attr.config;
      pe.config1 = attr.config1;
      pe.config2 = attr.config2;
      pe.type = attr.type;
      pe.size = attr.size;
      // attr.disabled = 1;
      pe.exclude_kernel = 1;
      pe.exclude_hv = 1;
      try {
        perf_fds.push_back(make_perf_fd(pe));
      } catch (const std::exception &e) {
        std::cerr << pmc.name() << ": " << e.what() << std::endl;
      }
    }
  }

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) override {
    for (const auto &perf_fd : perf_fds) {
      if (ioctl(static_cast<int>(*perf_fd), PERF_EVENT_IOC_RESET, 0)) {
        std::cerr << "Error in ioctl\n";
      }
    }

    return static_cast<gsl::span<uint64_t>::index_type>(perf_fds.size());
  }

  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t> buf) override {
    uint64_t v;
    int idx = 0;
    for (const auto &perf_fd : perf_fds) {
      const auto ret = read(static_cast<int>(*perf_fd), &v, sizeof(v));
      if (ret < 0) {
        std::cerr << "perf: Error reading performance counter: " << errno
                  << ": " << strerror(errno) << std::endl;
      } else if (ret != sizeof(v)) {
        std::cerr << "perf: Error reading " << sizeof(v) << " bytes."
                  << std::endl;
      }
      buf[idx] = v;
      ++idx;
    }
    return idx;
  }
};

template <typename MCK, typename LINUX>
class x86_linux_mckernel : public x86_64_base_pmu {
  std::unique_ptr<x86_pmc_base> backend_;

public:
  using resolver_type = upca::arch::jevents_resolver;

  template <typename T>
  x86_linux_mckernel(const T &pmcs)
      : backend_(mck_is_mckernel() ? static_cast<std::unique_ptr<x86_pmc_base>>(
                                         std::make_unique<MCK>(pmcs))
                                   : static_cast<std::unique_ptr<x86_pmc_base>>(
                                         std::make_unique<LINUX>(pmcs))) {}

  auto start(gsl::span<uint64_t> o) { return backend_->start(o); }
  auto stop(gsl::span<uint64_t> o) { return backend_->stop(o); }
};

#else

namespace {
static constexpr const char reason[] =
    "PMC support not compiled in; libjevents missing";
}

class x86_64_pmu : public x86_64_base_pmu {
public:
  using resolver_type = upca::arch::detail::null_resolver<reason>;
  template <typename T> x86_64_pmu(const T &) {}

  gsl::span<uint64_t>::index_type start(gsl::span<uint64_t>) {}
  gsl::span<uint64_t>::index_type stop(gsl::span<uint64_t>) {}
};

#endif /* JEVENTS_FOUND */

} // namespace x86_64
} // namespace arch
} // namespace upca


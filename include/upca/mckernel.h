#pragma once

#ifndef __APPLE__

#include <inttypes.h>
#include <unistd.h>

/*
SYSCALL_HANDLED(601, pmc_init)
SYSCALL_HANDLED(602, pmc_start)
SYSCALL_HANDLED(603, pmc_stop)
SYSCALL_HANDLED(604, pmc_reset)
*/

static int mck_is_mckernel() { return syscall(732) == 0; }

static int mck_pmc_init(int counter, int type, unsigned mode) {
  return syscall(601, counter, type, mode);
}

static int mck_pmc_start(unsigned long counter) { return syscall(602, counter); }
static int mck_pmc_stop(unsigned long counter) { return syscall(603, counter); }
static int mck_pmc_reset(int counter) { return syscall(604, counter); }

#else

static int mck_is_mckernel() { return 0; }

#endif


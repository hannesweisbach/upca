#pragma once

#include <memory>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>

#include <gsl/gsl>

#include <upca/config.h>

#include "arch/arch.h"

namespace upca {

template <typename BACKEND> class resolver {
  class description {

    std::string name_;
    unsigned size_;
    unsigned offset_;
    decltype(typename BACKEND::resolver_type().resolve({})) data_;

  public:
    description(std::string name, const unsigned size, const unsigned offset,
                const typename BACKEND::resolver_type &resolver)
        : name_(std::move(name)), size_(size), offset_(offset),
          data_(resolver.resolve(name_)) {}
    description(const char *name, const unsigned size, const unsigned offset,
                const typename BACKEND::resolver_type &resolver)
        : name_(name), size_(size), offset_(offset),
          data_(resolver.resolve(name_)) {}

    const std::string &name() const { return name_; }
    unsigned size() const { return size_; }
    unsigned offset() const { return offset_; }
    decltype(auto) data() const { return data_; }
  };

  std::vector<description> counters_;
  typename BACKEND::resolver_type resolver_;

public:
  using value_type = description;
  using iterator = typename std::vector<description>::iterator;
  using const_iterator = typename std::vector<description>::const_iterator;

  void add(const char *const name, const unsigned size = 8) {
    counters_.emplace_back(name, size, counters_.size(), resolver_);
  }

  void add(std::string name, const unsigned size = 8) {
    counters_.emplace_back(std::move(name), size, counters_.size(), resolver_);
  }

  size_t size() const { return counters_.size() + 1; }

  unsigned bytesize() const {
    unsigned sum = 0;
    for (const auto &pmc : counters_) {
      sum += pmc.size();
    }
    return sum;
  }

  const description &
  at(const typename std::vector<description>::size_type pos) const {
    return counters_.at(pos);
  }
  const_iterator begin() const { return counters_.cbegin(); }
  const_iterator end() const { return counters_.cend(); }
  const_iterator cbegin() const { return counters_.cbegin(); }
  const_iterator cend() const { return counters_.cend(); }

  /* Get PMU backend. Thread has to be pinned to a CPU before this is called */
  std::unique_ptr<BACKEND> configure(const unsigned additional_counters = 0) const {
    /* meh. add +1 for the "out-of-band" cycles counter :/ */
    return std::make_unique<BACKEND>(counters_, additional_counters + 1);
  }
};

/* sample pmcs */
} // namespace upca

#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>

#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace sortlab {

enum class PerfMetric {
  cycles,
  instructions,
  branches,
  branch_misses,
  cache_references,
  cache_misses,
};

class PerfEvent {
 public:
  explicit PerfEvent(PerfMetric metric) { open(metric); }
  ~PerfEvent() { close(); }
  PerfEvent(const PerfEvent&) = delete;
  PerfEvent& operator=(const PerfEvent&) = delete;

  [[nodiscard]] bool available() const { return fd_ >= 0; }
  [[nodiscard]] const std::string& reason() const { return reason_; }

  void start() {
#ifdef __linux__
    if (fd_ < 0) return;
    (void)::ioctl(fd_, PERF_EVENT_IOC_RESET, 0);
    (void)::ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
#endif
  }

  std::uint64_t stop() {
#ifdef __linux__
    if (fd_ < 0) return 0;
    (void)::ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
    return read_scaled();
#else
    return 0;
#endif
  }

 private:
  int fd_ = -1;
  std::string reason_;

#ifdef __linux__
  static std::uint64_t config(PerfMetric metric) {
    switch (metric) {
      case PerfMetric::cycles: return PERF_COUNT_HW_CPU_CYCLES;
      case PerfMetric::instructions: return PERF_COUNT_HW_INSTRUCTIONS;
      case PerfMetric::branches: return PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
      case PerfMetric::branch_misses: return PERF_COUNT_HW_BRANCH_MISSES;
      case PerfMetric::cache_references: return PERF_COUNT_HW_CACHE_REFERENCES;
      case PerfMetric::cache_misses: return PERF_COUNT_HW_CACHE_MISSES;
    }
    return PERF_COUNT_HW_CPU_CYCLES;
  }

  void open(PerfMetric metric) {
    perf_event_attr attr{};
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config(metric);
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    fd_ = static_cast<int>(::syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0));
    if (fd_ < 0) reason_ = std::strerror(errno);
  }

  std::uint64_t read_scaled() const {
    struct ReadResult {
      std::uint64_t value;
      std::uint64_t time_enabled;
      std::uint64_t time_running;
    } result{};
    if (::read(fd_, &result, sizeof(result)) != static_cast<ssize_t>(sizeof(result)) ||
        result.time_running == 0) {
      return 0;
    }
    if (result.time_running == result.time_enabled) return result.value;
    const long double scaled = static_cast<long double>(result.value) *
                               static_cast<long double>(result.time_enabled) /
                               static_cast<long double>(result.time_running);
    return static_cast<std::uint64_t>(scaled + 0.5L);
  }

  void close() {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }
#else
  void open(PerfMetric) { reason_ = "Linux perf_event_open is unavailable on this platform"; }
  void close() {}
#endif
};

}  // namespace sortlab

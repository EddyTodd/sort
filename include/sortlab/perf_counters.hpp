#pragma once

#include <array>
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

struct PerfValues {
  bool available = false;
  std::uint64_t cycles = 0;
  std::uint64_t instructions = 0;
  std::uint64_t branches = 0;
  std::uint64_t branch_misses = 0;
  std::uint64_t cache_references = 0;
  std::uint64_t cache_misses = 0;
};

class PerfCounters {
 public:
  PerfCounters() { open_all(); }
  ~PerfCounters() { close_all(); }
  PerfCounters(const PerfCounters&) = delete;
  PerfCounters& operator=(const PerfCounters&) = delete;

  [[nodiscard]] bool available() const { return available_; }
  [[nodiscard]] const std::string& reason() const { return reason_; }

  void start() {
#ifdef __linux__
    if (!available_) return;
    for (const int fd : fds_) {
      (void)::ioctl(fd, PERF_EVENT_IOC_RESET, 0);
      (void)::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
#endif
  }

  PerfValues stop() {
    PerfValues result;
    result.available = available_;
#ifdef __linux__
    if (!available_) return result;
    for (const int fd : fds_) (void)::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    std::array<std::uint64_t, 6> values{};
    for (std::size_t i = 0; i < fds_.size(); ++i) values[i] = read_scaled(fds_[i]);
    result.cycles = values[0];
    result.instructions = values[1];
    result.branches = values[2];
    result.branch_misses = values[3];
    result.cache_references = values[4];
    result.cache_misses = values[5];

    if (result.cycles == 0 && result.instructions == 0) {
      result.available = false;
      available_ = false;
      reason_ = "perf events opened but returned zero counts";
    }
#endif
    return result;
  }

 private:
  bool available_ = false;
  std::string reason_;

#ifdef __linux__
  std::array<int, 6> fds_{{-1, -1, -1, -1, -1, -1}};

  static int open_event(std::uint64_t config) {
    perf_event_attr attr{};
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    return static_cast<int>(::syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0));
  }

  void open_all() {
    constexpr std::array<std::uint64_t, 6> configs = {
        PERF_COUNT_HW_CPU_CYCLES,       PERF_COUNT_HW_INSTRUCTIONS,
        PERF_COUNT_HW_BRANCH_INSTRUCTIONS, PERF_COUNT_HW_BRANCH_MISSES,
        PERF_COUNT_HW_CACHE_REFERENCES, PERF_COUNT_HW_CACHE_MISSES};
    for (std::size_t i = 0; i < configs.size(); ++i) {
      fds_[i] = open_event(configs[i]);
      if (fds_[i] < 0) {
        reason_ = std::strerror(errno);
        close_all();
        return;
      }
    }
    available_ = true;
  }

  static std::uint64_t read_scaled(int fd) {
    struct ReadResult {
      std::uint64_t value;
      std::uint64_t time_enabled;
      std::uint64_t time_running;
    } result{};
    if (::read(fd, &result, sizeof(result)) != static_cast<ssize_t>(sizeof(result)) ||
        result.time_running == 0) {
      return 0;
    }
    if (result.time_running == result.time_enabled) return result.value;
    const long double scaled = static_cast<long double>(result.value) *
                               static_cast<long double>(result.time_enabled) /
                               static_cast<long double>(result.time_running);
    return static_cast<std::uint64_t>(scaled + 0.5L);
  }

  void close_all() {
    for (int& fd : fds_) {
      if (fd >= 0) {
        ::close(fd);
        fd = -1;
      }
    }
    available_ = false;
  }
#else
  void open_all() { reason_ = "Linux perf_event_open is unavailable on this platform"; }
  void close_all() {}
#endif
};

}  // namespace sortlab

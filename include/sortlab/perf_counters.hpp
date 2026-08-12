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
struct PerfValues{bool available=false;std::uint64_t cycles=0,instructions=0,branches=0,branch_misses=0,cache_references=0,cache_misses=0;};
class PerfCounters{
 public:
  PerfCounters(){open_all();}
  ~PerfCounters(){close_all();}
  PerfCounters(const PerfCounters&)=delete;PerfCounters& operator=(const PerfCounters&)=delete;
  bool available()const{return available_;}const std::string& reason()const{return reason_;}
  void start(){
#ifdef __linux__
    if (!available_) return;
    for (int fd : fds_) {
      (void)::ioctl(fd, PERF_EVENT_IOC_RESET, 0);
      (void)::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
#endif
  }
  PerfValues stop(){PerfValues out;out.available=available_;
#ifdef __linux__
    if (!available_) return out;
    for (int fd : fds_) (void)::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    std::array<std::uint64_t,6> v{};
    for (std::size_t i=0;i<fds_.size();++i) v[i]=read_scaled(fds_[i]);
    out.cycles=v[0];out.instructions=v[1];out.branches=v[2];out.branch_misses=v[3];out.cache_references=v[4];out.cache_misses=v[5];
    if (out.cycles == 0 && out.instructions == 0) {
      out.available = false;
      available_ = false;
      reason_ = "perf events opened but returned zero counts";
    }
#endif
    return out;}
 private:
  bool available_=false;std::string reason_;
#ifdef __linux__
  std::array<int,6>fds_{{-1,-1,-1,-1,-1,-1}};
  static int open_event(std::uint64_t config){perf_event_attr attr{};attr.type=PERF_TYPE_HARDWARE;attr.size=sizeof(attr);attr.config=config;attr.disabled=1;attr.exclude_kernel=1;attr.exclude_hv=1;attr.read_format=PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING;return static_cast<int>(::syscall(SYS_perf_event_open,&attr,0,-1,-1,0));}
  void open_all(){const std::array<std::uint64_t,6>cfg={PERF_COUNT_HW_CPU_CYCLES,PERF_COUNT_HW_INSTRUCTIONS,PERF_COUNT_HW_BRANCH_INSTRUCTIONS,PERF_COUNT_HW_BRANCH_MISSES,PERF_COUNT_HW_CACHE_REFERENCES,PERF_COUNT_HW_CACHE_MISSES};for(std::size_t i=0;i<cfg.size();++i){fds_[i]=open_event(cfg[i]);if(fds_[i]<0){reason_=std::strerror(errno);close_all();return;}}available_=true;}
  static std::uint64_t read_scaled(int fd){struct Read{std::uint64_t value,time_enabled,time_running;}r{};if(::read(fd,&r,sizeof(r))!=static_cast<ssize_t>(sizeof(r))||r.time_running==0)return 0;if(r.time_running==r.time_enabled)return r.value;const long double scaled=static_cast<long double>(r.value)*static_cast<long double>(r.time_enabled)/static_cast<long double>(r.time_running);return static_cast<std::uint64_t>(scaled+0.5L);}
  void close_all(){for(int&fd:fds_)if(fd>=0){::close(fd);fd=-1;}available_=false;}
#else
  void open_all(){reason_="Linux perf_event_open is unavailable on this platform";}
  void close_all(){}
#endif
};
}

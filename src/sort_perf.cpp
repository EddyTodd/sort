#include "sortlab/extended_algorithms.hpp"
#include "sortlab/perf_counters.hpp"
#include "sortlab/workloads.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#ifdef __linux__
#include <sched.h>
#endif
using namespace sortlab;
static std::vector<std::string> split(std::string_view x){std::vector<std::string>o;std::size_t p=0;while(p<=x.size()){auto e=x.find(',',p);auto t=x.substr(p,e==std::string_view::npos?x.size()-p:e-p);if(t.empty())throw std::runtime_error("empty item");o.emplace_back(t);if(e==std::string_view::npos)break;p=e+1;}return o;}
static std::vector<std::size_t> nums(std::string_view x){std::vector<std::size_t>o;for(const auto&s:split(x))o.push_back(static_cast<std::size_t>(std::stoull(s)));return o;}
static std::vector<std::size_t> select(const std::vector<std::string>& names){const auto&t=all_algorithms();std::vector<std::size_t>o;if(names.empty()){o.resize(t.size());std::iota(o.begin(),o.end(),0);return o;}for(const auto&name:names){auto it=std::find_if(t.begin(),t.end(),[&](const Algorithm&a){return a.name==name;});if(it==t.end())throw std::runtime_error("unknown algorithm: "+name);o.push_back(static_cast<std::size_t>(std::distance(t.begin(),it)));}return o;}
static void pin_cpu(int cpu){if(cpu<0)return;
#ifdef __linux__
 cpu_set_t set;CPU_ZERO(&set);CPU_SET(cpu,&set);if(sched_setaffinity(0,sizeof(set),&set)!=0)throw std::runtime_error("sched_setaffinity failed");
#else
 throw std::runtime_error("--cpu requires Linux");
#endif
}
static int self_test(){auto input=make_data("random",257,trial_seed(1,"random",257,0));for(const auto&name:{"insertion","merge_insertion_24","dual_pivot","radix_lsd_11","std_sort"}){auto idx=select({name})[0];auto a=input;Stats s;all_algorithms()[idx].timed(a,s);if(!verify(input,a))return 1;}PerfCounters counters;std::cout<<"PASS: perf harness; counters_available="<<(counters.available()?1:0)<<'\n';return 0;}
int main(int argc,char**argv){try{bool test=false;int trials=11,cpu=-1;std::uint64_t seed=0x5EEDULL;std::vector<std::size_t>sizes={1024,16384,262144};std::vector<std::string>patterns={"random","few_unique","nearly_sorted"},names={"intro","merge_insertion_24","dual_pivot","radix_lsd_11","std_sort"};for(int i=1;i<argc;++i){std::string a=argv[i];if(a=="--self-test")test=true;else if(a=="--trials"&&i+1<argc)trials=std::stoi(argv[++i]);else if(a=="--cpu"&&i+1<argc)cpu=std::stoi(argv[++i]);else if(a=="--seed"&&i+1<argc)seed=std::stoull(argv[++i]);else if(a=="--sizes"&&i+1<argc)sizes=nums(argv[++i]);else if(a=="--patterns"&&i+1<argc)patterns=split(argv[++i]);else if(a=="--algorithms"&&i+1<argc)names=split(argv[++i]);else if(a=="--help"){std::cout<<"sort_perf [--cpu N] [--trials N] [--sizes csv] [--patterns csv] [--algorithms csv]\n";return 0;}else throw std::runtime_error("bad argument: "+a);}if(test)return self_test();if(trials<1)throw std::runtime_error("trials must be positive");pin_cpu(cpu);auto selected=select(names);const auto&t=all_algorithms();PerfCounters counters;if(!counters.available())std::cerr<<"hardware counters unavailable: "<<counters.reason()<<'\n';std::cout<<"schema_version,algorithm,pattern,n,trial,trial_seed,input_hash,execution_order,ns,perf_available,cycles,instructions,branches,branch_misses,cache_references,cache_misses,verified\n";for(auto n:sizes)for(const auto&p:patterns)for(int trial=0;trial<trials;++trial){auto ts=trial_seed(seed,p,n,static_cast<std::uint64_t>(trial));auto input=make_data(p,n,ts);auto fp=input_hash(input);auto order=selected;deterministic_shuffle(order,splitmix64(ts^0x94D049BB133111EBULL));std::size_t ord=0;for(auto idx:order){auto a=input;Stats s;counters.start();auto st=std::chrono::steady_clock::now();t[idx].timed(a,s);auto en=std::chrono::steady_clock::now();auto perf=counters.stop();bool ok=verify(input,a);std::cout<<1<<','<<t[idx].name<<','<<p<<','<<n<<','<<trial<<','<<ts<<','<<fp<<','<<ord++<<','<<std::chrono::duration_cast<std::chrono::nanoseconds>(en-st).count()<<','<<(perf.available?1:0)<<','<<perf.cycles<<','<<perf.instructions<<','<<perf.branches<<','<<perf.branch_misses<<','<<perf.cache_references<<','<<perf.cache_misses<<','<<(ok?1:0)<<'\n';if(!ok)return 1;}}return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<'\n';return 2;}}

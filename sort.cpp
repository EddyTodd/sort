#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct Stats { std::uint64_t comparisons=0, swaps=0, writes=0; };
using Value = std::int64_t;
using SortFn = std::function<void(std::vector<Value>&, Stats&)>;

static bool less(Value a, Value b, Stats& s){ ++s.comparisons; return a < b; }
static void swapv(Value& a, Value& b, Stats& s){ if (&a!=&b){ std::swap(a,b); ++s.swaps; s.writes+=2; } }

static void insertion_sort(std::vector<Value>& a, Stats& s){
  for(std::size_t i=1;i<a.size();++i){ Value key=a[i]; std::size_t j=i; ++s.writes;
    while(j>0){ ++s.comparisons; if(!(key<a[j-1])) break; a[j]=a[j-1]; ++s.writes; --j; }
    a[j]=key; ++s.writes;
  }
}
static void selection_sort(std::vector<Value>& a, Stats& s){
  for(std::size_t i=0;i<a.size();++i){ std::size_t m=i; for(std::size_t j=i+1;j<a.size();++j) if(less(a[j],a[m],s)) m=j; swapv(a[i],a[m],s); }
}
static void bubble_sort(std::vector<Value>& a, Stats& s){
  for(std::size_t n=a.size();n>1;--n){ bool changed=false; for(std::size_t i=1;i<n;++i){ if(less(a[i],a[i-1],s)){ swapv(a[i],a[i-1],s); changed=true; }} if(!changed) break; }
}
static void sift_down(std::vector<Value>& a,std::size_t root,std::size_t n,Stats& s){
  for(;;){ std::size_t child=root*2+1; if(child>=n) return; if(child+1<n && less(a[child],a[child+1],s)) ++child; if(!less(a[root],a[child],s)) return; swapv(a[root],a[child],s); root=child; }
}
static void heap_sort(std::vector<Value>& a, Stats& s){
  for(std::size_t i=a.size()/2;i>0;--i) sift_down(a,i-1,a.size(),s);
  for(std::size_t n=a.size();n>1;--n){ swapv(a[0],a[n-1],s); sift_down(a,0,n-1,s); }
}
static void merge_rec(std::vector<Value>& a,std::vector<Value>& tmp,std::size_t lo,std::size_t hi,Stats& s){
  if(hi-lo<2) return;
  auto mid=lo+(hi-lo)/2; merge_rec(a,tmp,lo,mid,s); merge_rec(a,tmp,mid,hi,s);
  std::size_t i=lo,j=mid,k=lo; while(i<mid && j<hi){ ++s.comparisons; if(a[j]<a[i]) tmp[k++]=a[j++]; else tmp[k++]=a[i++]; ++s.writes; }
  while(i<mid){ tmp[k++]=a[i++]; ++s.writes; } while(j<hi){ tmp[k++]=a[j++]; ++s.writes; }
  for(k=lo;k<hi;++k){ a[k]=tmp[k]; ++s.writes; }
}
static void merge_sort(std::vector<Value>& a, Stats& s){ std::vector<Value> tmp(a.size()); merge_rec(a,tmp,0,a.size(),s); }
static void quick_rec(std::vector<Value>& a,std::size_t lo,std::size_t hi,Stats& s){
  while(hi-lo>1){ std::size_t mid=lo+(hi-lo)/2; Value pivot=a[mid]; std::size_t i=lo,j=hi-1;
    for(;;){ while(i<hi && less(a[i],pivot,s)) ++i; while(j>lo && less(pivot,a[j],s)) --j; if(i>=j) break; swapv(a[i],a[j],s); ++i; if(j>0)--j; }
    std::size_t cut=i; if(cut<=lo) cut=lo+1; if(cut>=hi) cut=hi-1;
    if(cut-lo < hi-cut){ quick_rec(a,lo,cut,s); lo=cut; } else { quick_rec(a,cut,hi,s); hi=cut; }
  }
}
static void quick_sort(std::vector<Value>& a, Stats& s){ if(!a.empty()) quick_rec(a,0,a.size(),s); }
static void std_sort(std::vector<Value>& a, Stats& s){ std::sort(a.begin(),a.end(),[&](Value x,Value y){++s.comparisons;return x<y;}); }
static void std_stable_sort(std::vector<Value>& a, Stats& s){ std::stable_sort(a.begin(),a.end(),[&](Value x,Value y){++s.comparisons;return x<y;}); }

struct Algorithm { std::string name; SortFn fn; bool quadratic; };
static std::vector<Algorithm> algorithms(){ return {
  {"insertion",insertion_sort,true},{"selection",selection_sort,true},{"bubble",bubble_sort,true},
  {"heap",heap_sort,false},{"merge",merge_sort,false},{"quick",quick_sort,false},{"std_sort",std_sort,false},{"std_stable_sort",std_stable_sort,false}
}; }

static std::vector<Value> make_data(std::string_view pattern,std::size_t n,std::uint64_t seed){
  std::mt19937_64 rng(seed); std::vector<Value> a(n); std::uniform_int_distribution<Value> dist(-1000000000LL,1000000000LL);
  for(auto& x:a) x=dist(rng);
  if(pattern=="sorted") std::sort(a.begin(),a.end());
  else if(pattern=="reversed"){ std::sort(a.begin(),a.end(),std::greater<>()); }
  else if(pattern=="few_unique"){ std::uniform_int_distribution<int> d(0,7); for(auto& x:a)x=d(rng); }
  else if(pattern=="nearly_sorted"){ std::sort(a.begin(),a.end()); if(n>1){ auto swaps=std::max<std::size_t>(1,n/100); std::uniform_int_distribution<std::size_t>d(0,n-1); for(std::size_t k=0;k<swaps;++k) std::swap(a[d(rng)],a[d(rng)]); }}
  else if(pattern!="random") throw std::runtime_error("unknown pattern: "+std::string(pattern));
  return a;
}

static bool verify(const std::vector<Value>& input,const std::vector<Value>& output){ auto expected=input; std::sort(expected.begin(),expected.end()); return output==expected; }
static int self_test(){
  std::vector<std::vector<Value>> cases={{},{1},{2,1},{1,1,1},{3,-1,2,2,0},{5,4,3,2,1},{1,2,3,4,5}};
  std::mt19937_64 rng(1234567); for(int k=0;k<80;++k){ std::vector<Value> v(static_cast<std::size_t>(k)); for(auto& x:v)x=static_cast<Value>(rng()%31)-15; cases.push_back(std::move(v)); }
  for(const auto& alg:algorithms()) for(const auto& input:cases){ auto a=input; Stats s; alg.fn(a,s); if(!verify(input,a)){ std::cerr<<"FAIL "<<alg.name<<" n="<<input.size()<<'\n'; return 1; }}
  std::cout<<"PASS: "<<algorithms().size()<<" algorithms across "<<cases.size()<<" deterministic cases\n"; return 0;
}

static std::vector<std::size_t> parse_sizes(std::string_view text){ std::vector<std::size_t> out; std::size_t pos=0; while(pos<text.size()){ auto end=text.find(',',pos); auto token=text.substr(pos,end==std::string_view::npos?text.size()-pos:end-pos); out.push_back(static_cast<std::size_t>(std::stoull(std::string(token)))); if(end==std::string_view::npos)break; pos=end+1;} return out; }

int main(int argc,char** argv){
  bool do_test=false; std::uint64_t seed=0x5EEDULL; int trials=7; std::vector<std::size_t> sizes={32,256,2048,16384,131072};
  for(int i=1;i<argc;++i){ std::string arg=argv[i]; if(arg=="--self-test")do_test=true; else if(arg=="--seed"&&i+1<argc)seed=std::stoull(argv[++i]); else if(arg=="--trials"&&i+1<argc)trials=std::stoi(argv[++i]); else if(arg=="--sizes"&&i+1<argc)sizes=parse_sizes(argv[++i]); else if(arg=="--help"){ std::cout<<"sort_lab [--self-test] [--seed N] [--trials N] [--sizes a,b,c]\n"; return 0; } else { std::cerr<<"unknown argument: "<<arg<<'\n'; return 2; }}
  if(do_test)return self_test();
  if(trials<1){std::cerr<<"trials must be positive\n";return 2;}
  const std::vector<std::string> patterns={"random","sorted","reversed","few_unique","nearly_sorted"};
  std::cout<<"algorithm,pattern,n,trial,seed,ns,comparisons,swaps,writes,verified\n";
  for(auto n:sizes) for(const auto& pattern:patterns){ auto input=make_data(pattern,n,seed ^ (static_cast<std::uint64_t>(n)*0x9E3779B97F4A7C15ULL) ^ std::hash<std::string>{}(pattern));
    for(const auto& alg:algorithms()){
      if(alg.quadratic && n>16384) continue;
      for(int t=0;t<trials;++t){ auto a=input; Stats s; auto start=std::chrono::steady_clock::now(); alg.fn(a,s); auto stop=std::chrono::steady_clock::now(); auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count(); bool ok=verify(input,a);
        std::cout<<alg.name<<','<<pattern<<','<<n<<','<<t<<','<<seed<<','<<ns<<','<<s.comparisons<<','<<s.swaps<<','<<s.writes<<','<<(ok?1:0)<<'\n'; if(!ok)return 1;
      }
    }
  }
}

#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>
namespace sortlab::allocation_tracker {
struct Snapshot{std::size_t calls=0,total_requested=0,peak_live=0,max_single=0,live_at_stop=0;};
struct alignas(std::max_align_t) Header{std::size_t size;bool tracked;};
inline thread_local bool active=false;
inline thread_local Snapshot current{};
inline thread_local std::size_t live=0;
inline void start(){current={};live=0;active=true;}
inline Snapshot stop(){active=false;current.live_at_stop=live;return current;}
inline void on_allocate(std::size_t n,bool tracked){if(!tracked)return;++current.calls;current.total_requested+=n;current.max_single=std::max(current.max_single,n);live+=n;current.peak_live=std::max(current.peak_live,live);}
inline void on_deallocate(const Header&h){if(h.tracked){if(h.size<=live)live-=h.size;else live=0;}}
}
[[gnu::noinline]] inline void* operator new(std::size_t n){using namespace sortlab::allocation_tracker;const std::size_t bytes=sizeof(Header)+(n==0?1:n);void* raw=std::malloc(bytes);if(!raw)throw std::bad_alloc();auto* h=static_cast<Header*>(raw);h->size=n;h->tracked=active;on_allocate(n,h->tracked);return static_cast<void*>(h+1);}
[[gnu::noinline]] inline void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] inline void operator delete(void* p) noexcept{if(!p)return;using namespace sortlab::allocation_tracker;auto*h=static_cast<Header*>(p)-1;on_deallocate(*h);std::free(h);}
[[gnu::noinline]] inline void operator delete[](void* p) noexcept {if(!p)return;using namespace sortlab::allocation_tracker;auto*h=static_cast<Header*>(p)-1;on_deallocate(*h);std::free(h);}
[[gnu::noinline]] inline void operator delete(void* p,std::size_t) noexcept {if(!p)return;using namespace sortlab::allocation_tracker;auto*h=static_cast<Header*>(p)-1;on_deallocate(*h);std::free(h);}
[[gnu::noinline]] inline void operator delete[](void* p,std::size_t) noexcept {if(!p)return;using namespace sortlab::allocation_tracker;auto*h=static_cast<Header*>(p)-1;on_deallocate(*h);std::free(h);}

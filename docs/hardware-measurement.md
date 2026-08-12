# Hardware, allocation, and host-control measurements

Wall time can show that two algorithms differ; it cannot by itself identify the mechanism. This layer adds direct measurements while keeping platform-specific evidence separate from the portable benchmark contract.

## Hardware counters

`sort_perf` uses Linux `perf_event_open` around only the sorting call. Wall time is collected in a separate counter-free pass; each hardware event is then collected in its own fresh sort pass over the identical input. This design avoids forcing six events into one hardware counter group and avoids kernel multiplexing as a hidden comparison variable. It requests:

- CPU cycles;
- retired instructions;
- branch instructions;
- branch misses;
- cache references;
- cache misses.

Example:

```sh
./build/sort_perf --cpu 2 \
  --algorithms intro,merge_insertion_24,dual_pivot,radix_lsd_11,std_sort \
  --patterns random,few_unique,nearly_sorted \
  --sizes 1024,16384,262144 --trials 31 > perf.csv
python3 tools/analyze_perf.py perf.csv --output perf-summary.csv
```

Linux permissions, virtual machines, containers, and some cloud environments may deny or virtualize performance counters. Availability is recorded **per event**; unavailable or unusable events are not interpreted as zero work.

Because different event types are measured on separate executions, ratios such as CPI or miss rate combine paired same-input observations rather than simultaneous counts. They are diagnostics, not universal cost functions, and should be interpreted with the host manifest.

## CPU affinity

`sort_perf --cpu N` pins the process on Linux. The generic experiment wrapper also accepts `--cpu N` before the benchmark executable and pins the child process, allowing the scalar, record, cutoff, allocation, or perf harness to inherit a fixed affinity.

Affinity controls migration but not interrupts, SMT sibling activity, turbo/frequency changes, thermal throttling, or other system load.

## Allocation measurement

`sort_alloc` is a **separate allocation-only executable**. It overrides ordinary C++ `new`/`delete` only in that executable and enables tracking immediately around the sorting call. It reports:

- allocation calls;
- total requested bytes;
- peak simultaneously-live requested bytes;
- largest single request;
- live tracked bytes at stop.

This captures project vector buffers and ordinary allocations made by library sorts such as `std::stable_sort` when the implementation uses `new`. It does not claim to intercept `malloc` called directly by arbitrary external code, custom allocators, over-aligned allocation paths, kernel memory, stack space, page faults, or physical RSS.

The allocation executable must not be used for canonical timing because the tracking allocator intentionally changes allocation mechanics.

## Host manifest

`tools/run_experiment.py` now records the benchmark binary hash as well as raw-data hash. It captures pre/post host-control snapshots, including available CPU affinity, load averages, Linux governor/perf settings or macOS CPU/thermal metadata, and relevant allocator/thread environment variables.

Example:

```sh
python3 tools/run_experiment.py --cpu 2 --settle-ms 1000 \
  ./build/sort_lab results/canonical-001 -- \
  --trials 51 --warmups 2 --sizes 64,1024,16384,262144
```

The wrapper records state; it does not silently reconfigure frequency governors, turbo, thermal policies, or OS security settings.

## Mechanism-claim gate

A statement such as "algorithm A is slower because of branch misses" requires branch-counter evidence collected on the same scoped experiment. A statement about allocation requires allocation evidence. A statement about memory bandwidth requires a bandwidth-capable counter/tool; explicit move counts are insufficient.

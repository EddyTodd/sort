# External baseline track

Third-party sorters are **not vendored into Git history**. `baselines.json` pins repository, full commit, license, required paths, and adapter identity. `tools/bootstrap_external.py` materializes exactly those commits under `external/vendor/`, and `tools/verify_external.py` records license/tree hashes before evidence collection.

Current track:

- `orlp/pdqsort` @ `b1ef26a55cdb60d236a5cb199c4234c704f46726`, zlib license: `pdqsort` and `pdqsort_branchless`.
- `ips4o/ips4o` @ `08a5b926ee65cef19139057c6bde02bb5542c1cb`, BSD-2-Clause: sequential `ips4o::sort` only.

Bootstrap and verify:

```sh
python3 tools/bootstrap_external.py
python3 tools/verify_external.py --output external/provenance.json
cmake -S . -B build-external -DCMAKE_BUILD_TYPE=Release -DSORTLAB_ENABLE_EXTERNAL_BASELINES=ON
cmake --build build-external -j
ctest --test-dir build-external --output-on-failure
```

CMake never downloads these dependencies implicitly. This makes network activity auditable and prevents a floating upstream branch from silently changing a benchmark binary.

The IPS4o parallel API is deliberately excluded from this single-threaded track. Parallel sorting requires its own thread-count, affinity, NUMA, scheduler, and scaling contract.

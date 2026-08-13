# Repository layout

`sortlab` is a header-only C++23 library and follows the shared subject-repository contract defined by `EddyTodd/bench`.

## Permanent library

- `include/sortlab/` — installed public algorithms, metadata, observers, and implementation details required by the header-only API.
- `tests/` — deterministic permanent correctness/API/contract tests.
- `docs/` — algorithm theory, public contracts, scope, and retained sorting-specific research documentation.
- `cmake/` — package configuration and optional research integrations.

There is intentionally no permanent `src/` directory: `sortlab::sortlab` is an `INTERFACE` target.

## Transitional research

- `research/include/sortlab/` — pre-v1 benchmark-only algorithms, workloads, record types, and instrumentation retained for historical identity.
- `research/apps/` — historical benchmark/research executables.
- `tools/` — historical analysis, campaign, evidence, tuning, and external-bootstrap scripts pending migration/classification.
- `campaigns/`, `claims/`, and historical result conventions — retained empirical state pending `bench` migration.

Moving the executable sources under `research/` is a byte-preserving source-tree sanitation step. It does not authorize deleting a study or its evidence.

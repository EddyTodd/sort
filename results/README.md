# Benchmark results

This directory is reserved for **real, reproducible research artifacts**. Synthetic, guessed, copied, or development-only performance numbers do not belong here.

Single-run artifacts use:

```text
<run-id>/manifest.json
<run-id>/raw.csv
<run-id>/summary.csv
<run-id>/NOTES.md                                # optional
<run-id>/claims.csv                              # optional
<run-id>/crossovers.csv                          # optional
<run-id>/cutoff-summary.csv                      # optional, must preserve train/evaluation split
<run-id>/tiny-summary.csv                        # optional, paired tiny-kernel reduction
<run-id>/leaf-tuning.csv                         # optional, held-out kernel+cutoff evaluation
<run-id>/merge-policy-summary.csv                # optional, paired scheduler/minrun reduction
<run-id>/merge-kernel-summary.csv                # optional, paired buffer/gallop reduction
<run-id>/gallop-heldout.csv                      # optional, held-out scalar gallop-threshold evaluation
<run-id>/adaptive-record-policy-summary.csv      # optional, paired record scheduler/minrun reduction
<run-id>/adaptive-record-kernel-summary.csv      # optional, paired record buffer/gallop reduction
<run-id>/adaptive-record-policy-width-effects.csv
<run-id>/adaptive-record-kernel-width-effects.csv
<run-id>/adaptive-record-gallop-heldout.csv      # held-out threshold evaluation by payload width
<run-id>/portfolio-summary.csv                   # optional, held-out selector evaluation
```

Versioned multi-run campaigns use:

```text
campaigns/<campaign-id>/campaign-manifest.json
campaigns/<campaign-id>/<experiment-id>/rep-001/manifest.json
campaigns/<campaign-id>/<experiment-id>/rep-001/raw.csv
campaigns/<campaign-id>/<experiment-id>/rep-001/<derived analysis files>
...
```

The committed experiment contract lives in `campaigns/*.json`. `campaign-manifest.json` records its content hash and the status of every planned repetition. `tools/validate_artifacts.py` must pass before a run is attached to `claims/registry.json` as evidence.

Tiny-kernel direct results and integrated leaf-hybrid results remain separate artifacts because they answer different claims. A leaf treatment selected from training data must retain its held-out evaluation output rather than publishing the training winner alone.

Adaptive merge scheduler/minrun results and two-run merge-kernel results also remain separate artifacts. `merge-policies-v1` changes the merge tree while holding the merge kernel fixed; `merge-kernels-v1` freezes the scheduler/minrun context while changing buffering and galloping. Combining those raw treatments into one apparent factorial experiment would misrepresent the preregistered contracts.

`adaptive-records-v1` follows the same separation over records. Its policy and kernel subexperiments are separate raw evidence even though they share one executable and record schema. Record-width effects pair observations by treatment/workload/size/trial/**key hash**, because the payload bytes—and therefore the full input hash—intentionally change with payload width.

Every adaptive-record artifact accepted for analysis must have both `verified=1` and `stable_on_trial=1`. A sorted output that reorders equal-key records is a failed trial, not a lower-stability performance datapoint.

A gallop threshold selected from training observations must retain its held-out artifact. The training-selected threshold alone is not evidence that galloping improves held-out performance or that one threshold transfers across payload widths or environments.

Explicit record moves, explicit bytes moved, requested temporary bytes, and reusable-vector capacity are algorithmic observables. They are not direct cache-line, DRAM-bandwidth, RSS, or allocator-traffic measurements.

Hardware-counter and allocation campaigns retain distinct experiment IDs because their measurement contracts differ from canonical wall-time runs. A platform skip is absence of that measurement, never a zero-valued observation.

Before any benchmark statement is promoted into the project README, a paper, or a portfolio, it must satisfy the publication gate in `docs/research-protocol.md` and the claim-specific requirements in `claims/registry.json`.

Large campaigns may move raw datasets to release assets or an archival repository. If so, this directory must retain schema version, content hashes, exact source/binary identity, analysis-code version, campaign-spec hash, and durable artifact references.

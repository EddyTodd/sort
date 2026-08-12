# Campaign specifications

Campaign JSON files are versioned experiment contracts. They define **what to run before the final data are collected**, rather than reconstructing an experiment from shell history afterward.

- `smoke.json` is a fast end-to-end pipeline check. Its results are not canonical research evidence.
- `canonical-v1.json` is the first Tier-2 campaign contract for the portable core. It uses two independent repetitions per experiment and fixed analysis commands.
- `external-v1.json` is a separate Tier-2 contract for provenance-pinned engineered external baselines. It requires an external-enabled build produced only after `tools/bootstrap_external.py` verifies the pinned source checkouts.
- `tiny-kernels-v1.json` is the Tier-2 contract for direct small-set kernels and jointly tuned hybrid leaf kernels/cutoffs. It is the canonical evidence path for H12-H13.
- `merge-policies-v1.json` is the Tier-2 factorial contract for H14-H15: three stable adaptive merge schedules crossed with three minrun policies using a common merge kernel.

Run dry plans first:

```sh
python3 tools/campaign.py campaigns/canonical-v1.json --binary-dir build --dry-run
python3 tools/campaign.py campaigns/tiny-kernels-v1.json --binary-dir build --dry-run
python3 tools/campaign.py campaigns/merge-policies-v1.json --binary-dir build --dry-run
python3 tools/campaign.py campaigns/external-v1.json --binary-dir build-external --dry-run
```

Then execute on a controlled host:

```sh
python3 tools/campaign.py campaigns/canonical-v1.json \
  --binary-dir build --output-root results/campaigns \
  --cpu 2 --settle-ms 1000 --resume

python3 tools/campaign.py campaigns/tiny-kernels-v1.json \
  --binary-dir build --output-root results/campaigns-tiny \
  --cpu 2 --settle-ms 1000 --resume

python3 tools/campaign.py campaigns/merge-policies-v1.json \
  --binary-dir build --output-root results/campaigns-merge-policies \
  --cpu 2 --settle-ms 1000 --resume
```

For the external track, retain the generated provenance report beside the campaign artifacts and use a distinct output root/namespace:

```sh
python3 tools/bootstrap_external.py
python3 tools/verify_external.py --output external/provenance.json
cmake -S . -B build-external -DCMAKE_BUILD_TYPE=Release -DSORTLAB_ENABLE_EXTERNAL_BASELINES=ON
cmake --build build-external -j
python3 tools/campaign.py campaigns/external-v1.json \
  --binary-dir build-external --output-root results/campaigns-external \
  --cpu 2 --settle-ms 1000 --resume
```

`--resume` skips only runs whose raw CSV still matches the SHA-256 recorded in their manifest. Analysis steps are rerun for valid resumed data so derived artifacts can be regenerated without modifying raw evidence.

The tiny-kernel campaign deliberately keeps direct-kernel and integrated-hybrid experiments separate. A direct small-array winner is not automatically promoted to the preferred hybrid leaf. Joint leaf-kernel/cutoff tuning uses training trials, while the selected treatment is evaluated on held-out trials against the best insertion-only cutoff selected from the same training data.

The adaptive-merge campaign holds the stable merge kernel constant while varying scheduler and minrun policy. Structural merge cost/entropy is reported beside, not in place of, paired wall time. The preregistered `powersort + balanced` analysis baseline is a comparison reference, not a declared winner.

Platform-scoped experiments are explicit. The hardware-counter experiment is Linux-only and is recorded as `skipped-platform` elsewhere; this is not treated as a failed or zero-valued measurement.

A materially different compiler, standard library, CPU, build configuration, or external upstream commit should use a distinct output root or campaign namespace. Do not merge incompatible environments or dependency revisions into one raw campaign directory.

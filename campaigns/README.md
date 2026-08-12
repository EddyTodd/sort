# Campaign specifications

Campaign JSON files are versioned experiment contracts. They define **what to run before the final data are collected**, rather than reconstructing an experiment from shell history afterward.

- `smoke.json` is a fast end-to-end pipeline check. Its results are not canonical research evidence.
- `canonical-v1.json` is the first Tier-2 campaign contract for the portable core. It uses two independent repetitions per experiment and fixed analysis commands.
- `external-v1.json` is a separate Tier-2 contract for provenance-pinned engineered external baselines. It requires an external-enabled build produced only after `tools/bootstrap_external.py` verifies the pinned source checkouts.

Run a dry plan first:

```sh
python3 tools/campaign.py campaigns/canonical-v1.json --binary-dir build --dry-run
python3 tools/campaign.py campaigns/external-v1.json --binary-dir build-external --dry-run
```

Then execute on a controlled host:

```sh
python3 tools/campaign.py campaigns/canonical-v1.json \
  --binary-dir build --output-root results/campaigns \
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

Platform-scoped experiments are explicit. The hardware-counter experiment is Linux-only and is recorded as `skipped-platform` elsewhere; this is not treated as a failed or zero-valued measurement.

A materially different compiler, standard library, CPU, build configuration, or external upstream commit should use a distinct output root or campaign namespace. Do not merge incompatible environments or dependency revisions into one raw campaign directory.

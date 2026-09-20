# ACK 6.12.40 to 6.12.52 source-only handoff

> **History note (2026-09-19):** This file records the original two-parent
> source integration.  `main` now carries the same resolved kernel content as
> an individual first-parent replay; see
> [the linear-history reconciliation](ack-6.12.40-to-6.12.52-linear-history-reconciliation.md).

This is an **unbuilt experiment**, not a boot candidate or qualification.
The starting repository `main` was `912642ea0339d6f56fc3abc634d1be9051372046`.
The physically tested `.40` DRM repair on that branch is separate from the
older OOS 16.0.10.500 `.40` static-build report; confirm the device firmware
and exact stock payload contract again before any later physical test.

## Exact ACK ancestry

- Previous official tag: `android16-6.12.40_r00`, peeled commit
  `c026dd5eaf39a29cfac9ed1405778d420ec79939`.
- Target official tag: `android16-6.12.52_r00`, peeled commit
  `36e5f6313583daf75ec31104ee8a186798a3b95f`.
- Upstream interval: 2,298 Git commits, of which 2,255 are non-merge commits.
- KMI generation at the target: 5 (`build.config.constants`).
- Kernel Makefile version: 6.12.52. No version or release string was forged.

The source integration is a two-parent subtree merge, with the exact official
ACK tag as the second parent. Thus every official commit, author, subject, and
dependency edge remains reachable by its original SHA. It is **not** a squash
or a single synthetic patch. The first-parent merge boundaries for the twelve
completed stable integrations are:

| Version | Official merge SHA |
|---|---|
| 6.12.41 | `2b6b8dab9293a14987a301202ce2084591516d9c` |
| 6.12.42 | `f1f28b39325569ef9bfa49b9034e7ec78c02d8f2` |
| 6.12.43 | `a6cdc86803d3bcd160d223d4e07bc877d569782a` |
| 6.12.44 | `3d54fb16a2f413953a868095cf60a48898c3e707` |
| 6.12.45 | `f9517bb687fcd4fb816923806ee9777231d660a4` |
| 6.12.46 | `8ba28192acb550ac3ee5f777ed1568791def2337` |
| 6.12.47 | `0d5e90aca66051337123e69c032d863397a94048` |
| 6.12.48 | `2886932ff2f246e7fcae90dc6aabc37180c21383` |
| 6.12.49 | `cb0be73625fa56753b17f5c9a372b72950c81e8f` |
| 6.12.50 | `4b78f5af54523de4a63b23c158e1fb97a180c4d9` |
| 6.12.51 | `0f66b6009332e11491ef5bb16fc61895d36d04ca` |
| 6.12.52 | `36e5f6313583daf75ec31104ee8a186798a3b95f` |

No individual `.41`-`.44` or `.46`-`.51` point tag was invented. The manifest
pins only the exact `.52` official tag; OnePlus source pins are unchanged.

## Merge decisions and deferred gates

The subtree merge changed only `kernel_platform/common`. Its 29 textual
conflicts covered pKVM/Kconfig, Rust Binder, HID, UFS, device mapper, Android
vendor hooks, GKI symbols, and the generated ABI reference. The resolutions
retain downstream pKVM debugging/ASID checks, HID digitizer handling, UFS HID
sysfs support, tracepoint providers, and the zone-reset-all no-split guard;
they also incorporate new ACK helpers, Binder process lifetime handling,
vendor hooks, and zoned I/O behavior. These are source-level decisions only.

The generated `gki/aarch64/abi.stg` was mechanically reconciled without
changing enforcement. Its text has no duplicate IDs or dangling root symbol
links and retains all 9,245 starting elf-symbol records. It does **not** prove
the built ABI: 101 symbols in the official `.52` reference are not yet in the
reconciled downstream reference, and overlapping type records require a built
comparison. Do not hand-add those records or update the ABI reference merely
to silence a future diff. Review exact additions/removals/type changes first;
an ABI update, if needed, requires separate authorization.

The baseline used a hermetic SHA-512 signer, 103 signed GKI modules, stock
module binaries, and an OxygenOS Android-v4 boot-container recipe. None of
those results transfer automatically to this new source. Before packaging,
repeat the exact final-source gates from the `.40` static report:

```sh
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel run //common:kernel_aarch64_abi_dist -- --destdir=out/abi-6.12.52
tools/bazel build --kbuild_symtypes \
  //common:kernel_aarch64 \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //soc-repo:canoe_perf_abi
```

Then compare the final Canoe `.config` with the qualified `.40` config;
inspect the final ABI difference and all stock-module imports/CRCs, private
structure and protected-export contracts; verify the signer and every module
signature; and audit FBE, DM, UFS, Rust Binder, WLAN, Bluetooth, and DRM.
Only after these gates pass may the established Android-v4 Image-only boot
packaging procedure be repeated. Reuse neither an old Image nor old test
results. Do not change stock DLKMs, vendor_boot, DTBO, or VBMeta implicitly.

No kernel build, generated `.config`, ABI/KMI result, module audit, boot image,
phone test, push, release, or `main` promotion was performed for this `.52`
experiment.

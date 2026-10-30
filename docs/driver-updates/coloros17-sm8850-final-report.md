# ColorOS 17 SM8850 driver modernization: staged audit and first backport

Date: 2026-10-30. Target: OnePlus 15, `CPH2745_17.0.0.102(EX01)`.
This report does **not** qualify a replacement DLKM image or announce a release.
The released boot-only kernel and all original supporting images are unchanged.

## Baseline and isolation

Working branch: `experiment/coloros17-sm8850-dlkm-updates`.
Starting source: `efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c`.
The common source tree remains `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22`:
Linux 6.12.114, Android 16 KMI6, unchanged KernelSU/SUSFS, configuration,
watchdog protections, module policy and stable fixes.

The owner confirms that no exact OnePlus source archive/manifest is currently
available for this firmware. Exact shipping vendor Git revisions therefore
remain unverified. Public-source controls are compared module-by-module with
the shipping ELF instructions, exports, imports, private offsets and KCFI;
they are not relabeled as exact firmware source merely because they compile.
No LineageOS configuration, build target or deployment dependency is used.

Qualified rollback boot SHA256:
`1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3`.
Independent copies of this boot, both original DLKM images, vendor_boot,
config and Module.symvers were made and their six-entry manifest reverified.
See [baseline evidence](coloros17-qualified-baseline.md) for full hashes,
source/toolchain identities, AVB contract and preservation paths.

## Inventory and authoritative references

- [Complete module inventory](coloros17-module-inventory.md): 1,070
  partition-qualified files, comprising 103 system_dlkm, 461 vendor_dlkm and
  506 vendor_boot files; 702 canonical names. Duplicate copies are not
  collapsed. The historical runtime log loaded 691 names, not 1,032.
- [Built-in units](coloros17-built-in-providers.md): 643 modules.builtin
  records from the exact qualified kernel. This metadata does not prove every
  generic driver's hardware binding on Canoe.
- [Upstream comparison](coloros17-upstream-comparison.md) and its
  [27-row exact-reference ledger](coloros17-upstream-refs.tsv): the three
  relevant CLO vendor namespaces, OnePlus public repositories and the
  Lineage provenance locator. All 24 CLO Git endpoints were queried; browser
  access failures were not mistaken for Git unavailability.
- [Forty-candidate revalidation](coloros17-candidate-revalidation.tsv):
  24 require matched modules, four require firmware/userspace, six are not
  applicable to the specific absent source path, and six remain unresolved.
  These are deployment/source-fit classifications, not claims that all forty
  vulnerabilities are confirmed in the shipping binaries.

The newest same-family WLAN endpoint is 10900, Camera15300, Audio12500 and
LA.VENDOR16.2.0 r1 21500. Several apparently newer tags alias already reviewed
commits. Different release trains are not assumed compatible. Exact URLs,
tags and SHAs, including the corrected historical NXP pin, are in the ledger.
No directory wholesale replacement or firmware/proprietary-blob modification
was made.

All fifteen requested technology groups are covered by the comparison
matrix. WLAN parser/lifetime fixes, KGSL mmap fixes, Camera bounds/lifetime
fixes, DSP state/lifetime fixes, SecureMSM scatterlist guards, audio timeout
handling and IPA/RMNET fixes remain separate dependency/private-ABI reviews.
Source absence in one later path does not make an entire subsystem safe.

## First bounded implementation: RMNET core

The shipping `rmnet_core.ko` has directly identified instruction sequences
without the reviewed MAP packet-length/padding/cumulative-payload guards.
The exact binary and offsets are recorded in the
[Stage-A binary review](coloros17-stage-a-binary-review.md).
No CVE number or exploitability/severity score is invented for this fix.

The minimal source series is:

1. `4ee253465786329a1156de7aad9000e9452a841c` — explicitly use
   `queue_work(system_wq, ...)` at the sole aggregation timer scheduling
   site. This restores the real stock queue and permitted import. The two
   queues are not pointer aliases. No KMI list, signature or loader bypass
   is used.
2. `084e43e044d572f30bec18ac3f0523d57d1c1351` — original Qualcomm
   `ea47e85e97b72674a30e5846975c5a1eb04c6fde`, with upstream author/date,
   repository and Change-Id preserved. Relative stable patch-id matches
   `da5c34f9f14c793e7a502dd462d9584778d4bcb0`. It adds MAP/padding,
   IP/TCP header, NLO payload and whole-frame-before-delivery checks on both
   linear and nonlinear coalescing paths.
3. `b7c709d8cd1381837afd97f170af6c5b3190a078` — separate local follow-up:
   widen both local aggregate packet counters,
   require nonzero packet count for nonlinear GRO fast-forward, validate
   actual minimum header bytes before dereferencing, and enforce the
   documented legacy frag0 cloning contract without moving SKB data.

The follow-up does not change the wire interpretation of pkt_len, impose a
new per-NLO packet limit, require the whole aggregate to fit one fragment,
or change any public/private structure or callback. A legacy deaggregated
frame must fit its current first fragment; mixed-head legacy inputs that
the old clone branch could not correctly handle are rejected. The QMAPv5
nonlinear descriptor path remains separate and retains its checked reads.
Normal mobile-data, tethering and USB traffic still need physical regression
testing; static review is not a guarantee of firmware behavior.

Only three implementation files change:

```text
vendor/qcom/opensource/datarmnet/core/rmnet_descriptor.c
vendor/qcom/opensource/datarmnet/core/rmnet_handlers.c
vendor/qcom/opensource/datarmnet/core/rmnet_map_data.c
```

No external header, config, module load list, common-kernel implementation,
firmware, ABI reference or historical stable commit is modified.
These source repairs are **not active on the phone**: replacing the actual
vendor module is necessary. A boot.img-only rebuild cannot deploy this
RMNET module change while retaining the old driver binary.

## Build and interface evidence

The exact qualified modules_prepare/config/Symvers and manifest-matched
clang19/r536225 were used with normal generated DDK makefiles. Unchanged
CNSS-prealloc and RMNET controls established component-specific fit before
backporting. SDK dependency builds are build inputs only: no IPA, GSI,
SCM, RMNET-control or RMNET-memory replacement is proposed.

The first unchanged RMNET control exposed a real unsigned-import-policy
failure: new headers selected system_percpu_wq, which the frozen kernel
does not permit for unsigned vendor modules. Matching CRCs did not excuse
that failure. The one-site compatibility commit naturally restores the
permitted stock system_wq import and queue behavior.

Final module build evidence is under
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/`.
Run07 unstripped, unsigned experimental module SHA256:
`7fa0bfcf5d79443156687c245ea3e39b995f39e1b21d1f28370fb1dac3d54191`.
Its generated Module.symvers SHA256 remains
`b51acea3b4df97134d9d086b391220d2102ec9c19ca7a014fb833a496b7938c0`.
The final module DDK config is unchanged at
`a505d61200d8b6c1fc61aa2c2cb5cf0257063218a5cc5c04d993889efa5ca46f`.
The exact module path is
`control-rmnet-core/rmnet_core/run07/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`
under that evidence root. It is a build artifact, not a packaged release.
The 29-file evidence seal `final-rmnet-run07-evidence.sha256` verifies with
SHA256 `313b5ee18fa5d9ce4015fec336b0f903f3cfd3116eaebf2d136005225e6d7fb0`.

| Gate | Result / scope |
|---|---|
| Targeted final RMNET compile/link/modpost | PASS |
| Natural exports / versions / hard imports | 140 / 156 / 155, stock-matching identities and CRCs |
| Retained stock consumer edges | 56 matching edges |
| Private boundary review | 15 complete DWARF layouts unchanged; 25/25 callback KCFI identities match |
| Getter/ownership boundary review | Stock accessors/offset constraints and allocation sizes preserved |
| Compiled unsigned-import/protected-export policy | Independently checked, not inferred from CRC alone |
| Final complete hybrid oracle | PASS: 1,070 locations, 1,032 active compatible, 38 dormant |
| Clean full production common/Canoe build | NOT RERUN for this vendor-only stage |
| Strict KMI/common ABI/Canoe ABI generation | Qualified baseline results preserved, not claimed as fresh target runs |
| Enforced common GKI comparison | Existing FAIL/exit4 with38 reviewed differences; not overall PASS |
| Device validation of replacement | NOT TESTED |
| Installable image/AVB qualification | BLOCKED as below |

The final hybrid contains the new module as **both** consumer and provider,
not merely as an optional alternative to its stock provider. All other
module bytes and module-loading metadata remain stock. Results:

```text
Hard imports:                 60,701 / 60,701
CRC records:                  61,777 / 61,777
Missing providers:            0
CRC mismatches:               0
Namespace failures:           0
Active blockers:              0
Production-provider errors:   0
Unsigned-import policy errors: 0 (54,986 checked edges)
Protected-export collisions:  0
```

All 3,220 existing ambiguous-edge provider records and the dependency table
are unchanged from the qualified baseline. All 56 edges supplied by the
replacement core have only its actual vendor_dlkm provider; no SDK or old
stock core silently substitutes for the new binary. The vmlinux-first check
accounts for 54,785 edges with zero production-provider errors. Evidence:
`hybrid-oracle-rmnet-run07/` under the build evidence root.

The repository test command
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/driver-updates -p 'test_*.py' -v`
passes 40 tests: 17 inventory tests, 14 RMNET host models and nine source
contracts. Eleven in-memory mutation controls reject removed guards/narrowed
counters. One host test compiles the exact checksum-workaround predicate and
checks 524,288 combinations. Checkpatch reports zero errors/warnings for the
complete and local three-file patch. These are not real SKB, KASAN, race,
firmware or device tests. External module compilation skipped optional BTF
generation without vmlinux; that warning is disclosed, not a BTF PASS.

The [private ABI report](coloros17-rmnet-private-abi.md) documents the erased
hook registry, SKB control block, netdev_priv offsets, descriptors, callback
ownership and allocation-size constraints. The
[build/packaging ledger](coloros17-build-and-packaging-gates.md) distinguishes
control failures, harmless instruction changes, SDK-only differences and
actual loader policy. No CRC, vermagic, generated type output or enforcement
was manually forced. Retained stock signing/trust identities are preserved;
that is not a new cryptographic audit of every unchanged signature.

## Production package and AVB blocker

If deployment becomes possible, only the vendor_dlkm copy of rmnet_core
needs replacement at its original `lib/modules/rmnet_core.ko` location.
Its dependency names remain stock. All other vendor modules, all system
modules and the vendor_boot early/recovery modules remain unchanged.
The qualified boot is the matching kernel; no new kernel behavior/config
change is necessary for this candidate. No vendor.img or Lineage payload
is required.

However, the stock top-level vbmeta directly embeds both DLKM hashtree root
digests and is signed with the OEM key. Rebuilding vendor_dlkm changes its
root. Recreating an unsigned local footer cannot preserve that signed-parent
contract. Neither an OEM signing capability nor an independently verified
accepted alternative device verification policy has been established here.
Archived rooted green/locked properties are insufficient proof.

Consequently **no new DLKM image, boot candidate, installer or flashable
release package was produced**. Resolving the verification chain is a real
deployment gate, not permission to disable AVB or change vbmeta/rollback
indexes. A fresh read-only device/bootloader/fs_mgr inspection and review of
a legitimate installation contract are the next deployment investigation.
Production remains the existing boot-only release with untouched stock DLKM.

## Security, watchdog and remaining work

CVE-2026-80668 is already repaired in the preserved source by
`633ac4e5f56b64aa9cf80d5813c6fac82cf43d82`, based on
`b8b09dc2bf35a00d4e0556b5d6308c7b917ebda2`. The later five security fixes
also remain untouched. This vendor project neither reapplies nor claims
credit for those common-kernel repairs.

No evidence here establishes that generic DSP/Video updates fix the earlier
adci_thread, zram_comp or hfi_core_dbg_cl idle wait reports. The actual stock
providers and the existing stack-specific watchdog treatment are preserved.
No timeout increase, detector weakening or replacement of those modules was
made merely to silence reports.

The CNSS-prealloc control matches stock runtime sections and relocations,
but its stack-symbol buffer candidate affects DEBUG_LEAK-disabled tracking
and is not credited as new active protection. The CNSS2 GPIO patch requires
coherent ownership/unwind prerequisites; it was not partially imported.
Camera, WLAN, KGSL, SecureMSM and the remaining stages still require their
own bounded source/control/private-ABI validation, firmware review and
matched module closure. They are not deployed or declared fixed by this
report.

See [physical testing and rollback](coloros17-test-and-rollback-plan.md) for
the required FBE/networking/hardware/suspend/watchdog matrix and exact
restoration set. All experimental hardware rows remain NOT TESTED.

**Disposition: source backport staged and individually built; deployment and
full production qualification remain blocked/uncompleted. No merge, push,
flash or publication was performed.**

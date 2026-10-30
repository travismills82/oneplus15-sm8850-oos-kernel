# ColorOS 17.0.0.102 Qualcomm source/provenance comparison

Audit: 2026-10-30, against the clean starting source
`efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c` on
`experiment/coloros17-sm8850-dlkm-updates`. The original provenance scan was
read-only. Subsequent isolated RMNET source/build/static-compatibility results
are recorded below, not represented as deployment or physical PASS. No
production branch, qualified kernel Image, configuration or firmware was
changed; the released boot-only artifact remains untouched.

This project targets ColorOS, not LineageOS. Lineage history is used only to
locate original Qualcomm commits. No Lineage build target, configuration,
device tree, userspace, firmware or binary module is selected.

## Baseline and source-confidence boundary

The [preserved baseline](coloros17-qualified-baseline.md) establishes framework
Android17/SDK37, vendor Android16/SDK36, Linux6.12.114 and KMI6. The mixed
framework/vendor generation is intentional; ColorOS17 does not imply a new
kernel KMI or permission to import a newer Qualcomm release train wholesale.

The [exact stock inventory](coloros17-module-inventory.md) accounts for1070
partition-qualified modules, not1070 unique drivers. All967 vendor/early-boot
copies identify kernel `6.12.69-android16-6-o-gb79517a1cbef-4k`;491 additionally
record `g4808b15dc936`. These are useful source-search anchors, not complete
source/config/toolchain proof. The official OnePlus GitHub commit endpoints
for those source anchors returned422/not-found during baseline investigation.

Current source still derives from the original OnePlus snapshot plus bounded
reviewed imports: WLAN/CNSS `.053` in `6744a3f6bcf4226e37fa04475bd83a84fd3c3701`,
RMNET `.102` in `fdd9ce8fed3f3476f67c144bb5a44c6a60b101c7`, and the earlier
per-component lineage recorded in the historical .601 audit. Those labels do
not identify the shipping ColorOS .102 implementation. Matching WLAN version
`5.2.1.144V`, symbol CRCs or filenames is likewise not exact source proof.

## Authoritative sources and current refs

The [27-row ref inventory](coloros17-upstream-refs.tsv) records exact official
URLs, previous pins, live peeled tag commits, highest advertised same-family
tags, and distinct later release trains. Full raw advertisements are retained
locally under `out/coloros17-driver-audit-reference/refs/`.

All24 CodeLinaro Git queries succeeded within bounded25-second invocations.
The web interface separately returned HTTP403/robots restrictions; this is not
a Git-source access failure. All three vendor namespaces were inspected:

- `platform/vendor/qcom-opensource/`, including WLAN and Bluetooth;
- `platform/vendor/qcom/opensource/`, including GPU, audio, DSP and SecureMSM;
- `platform/vendor/opensource/`, including camera, display, video and IPA.

The additional NXP namespace was checked as well. Of24 historical upstream
pins,23 match the live peeled value. The historical NXP TSV contains
`1097486ae5ae6db16e405f77487c2afffbb546db`; the actual advertised20500/21500
peeled commit is `1097486a366f4b9e7c25d6993a9767a36a0c095c`. Use the verified
value; no claim is made that the old value was a valid historical reference
or that a tag was rewritten.

Official OnePlus15 refs remain unchanged:

| Repository | Branch | Tip |
|---|---|---|
| OnePlusOSS modules/devicetree | `oneplus/sm8850_b_16.0.0_oneplus_15` | `5ab2a689ff87d7d28c511f1762cf41c1b90d965a` |
| OnePlusOSS SoC | same | `fc30e54174d254ff7f33622a9278e4435f6718d2` |
| OnePlusOSS kernel_manifest | `oneplus/sm8850` | `ecdd4809cbd01fa5f509a4708347026f63498303` |

The current official `oneplus_15.xml` still selects the b16 OnePlus15
branches. No explicit ColorOS17/.102 source tag or branch was advertised by
these queries. The15T and Pad branches are different products, not upgrades
for Infiniti. A provenance-only check of Lineage's `lineage-24.0` head found
`08b3042484e2cfc03fbd9ccb5073d55d561c0a79`, the already reviewed fingerprint
notifier change; it is not a production source or build dependency.

### Same-family references versus different trains

Highest numeric tag within the previously reviewed family is a bounded
search result, not a claim to know the newest suitable firmware release.

| Component | Same-family advertised endpoint | Peeled commit / disposition |
|---|---|---|
| WLAN qcacld | `WLAN.LA.2.0.r3-10900-kaanapali.0` | `77095667660c9eaeafd58083065722a208c2247e`, new P2P parsing fix |
| WLAN host/platform/fw-api | same10900 tag | Same code as previously reviewed10800 for these three repos |
| Graphics | `GRAPHICS.LA.16.0.r1-10300-kaanapali.0` | `adf5712becd72bce1c6b426b41ed82545dd923bb`, unchanged |
| Display / MM | `DISPLAY.LA.6.0.r1-17700-kaanapali.0` | aliases17600 code in both repos |
| Camera | `CAMERA.LA.6.0.r1-15300-kaanapali.0` | `a895695349f3ca188289e8674f57aac82ac1016c`, newer camera fixes |
| Audio | `AUDIO_HANDSET.LA.11.0.r1-12500-kaanapali.0` | `2bc34f6d8b61628135c9895a9f50e64c7be05b40`, hardware/DT-specific change |
| Video | `VIDEO_HANDSET.LA.1.0.r1-12600-kaanapali.0` | `359c5e3e7f7fad9c62e507a70334e08d73fc200a`, unchanged |
| Bluetooth | `BTFM.LA.2.0.r1-08400-kaanapali.0` | `43e2452860389f7738d1435772366d1fbc8e24bb`, aliases08300 |
| DSP, IPA, RMNET, SecureMSM, SPU, Touch, fingerprint, Synx | `LA.VENDOR.16.2.0.r1-21500-kaanapali.0` | aliases prior21300/21400 code in these repos |

Distinct `LA.VENDOR.16.2.6.r1`, `LA.VENDOR.16.2.0.r5/r6`,
`GRAPHICS.LA.16.1.r7/r10`, `DISPLAY.LA.6.11.r1/r2`, and video r7/r10 refs
were also enumerated. Same SoC spelling is insufficient proof of firmware,
HAL, private-ABI or device-tree compatibility; none is imported wholesale.

## Fifteen technology-group source-fit matrix

Every row remains experimental and subject to the actual
[provider/load/signing gates](coloros17-build-and-packaging-gates.md).

| Group | Current source fit | Useful bounded references | Risk / decision |
|---|---|---|---|
| WLAN | OnePlus `.053`; several parser/statistics/lifetime fixes genuinely missing from source; some newer datapath paths absent | qcacld10900, host/platform10800; explicit old/new function review | Requires matched modules; no automatic four-tree replacement |
| Bluetooth | Stock GKI core/HIDP plus vendor BTFM; corrected kernel-source L2CAP is not necessarily the internal stock-module implementation | BTFM08400 aliases08300 | Unresolved; preserve real core/provider boundary and proprietary firmware |
| GPU/KGSL | Existing mmap/fault paths match old vulnerable patterns | Two10300 mmap fixes | Requires matched modules; private structure, DMA/mmap and userspace review first |
| Display/DRM/DSI/panel | Old rotator and VHM paths present; stock panel/HAL/DT remains authoritative | Display17700 aliases17600 | Requires firmware or userspace update; no panel/calibration or broad DRM replacement |
| Camera | Several old bounds/lifetime paths present; worker_ctx path absent; current CRM locking differs | Camera15200 fixes plus bounded15300 follow-up | Requires matched modules; monolithic camera and extension/private-type closure |
| Audio | Missing timeout compensation in existing audio-ext-clk path | Audio12400 fix;12500 property change separately assessed | Requires matched modules; GPR/PRM/DSP closure; no DT expansion |
| Video/HFI | SSR/firmware-loading PM paths exist; newer HFI IRQ source file absent | Video12600; Lineage IRQ patch is provenance only | Requires firmware or userspace update; no claim these fix intentional idle waits |
| DSP | Existing registration permits pre-create states before using user->device | DSP21300/21500 state fix | Requires matched modules; adapt to current locking/refcount/error paths |
| IPA/RMNET/network | Baseline MAP/NLO gaps; selected RMNET fix/local guards now statically validated; separate PM-notifier/IPA items remain unmodified | Data21300/21500 same-family fixes; isolated three-commit RMNET group below | Requires matched modules; IPA QMI/PA restrictions remain Requires firmware or userspace update |
| SecureMSM/SPU | Existing QCE/QCrypto scatterlist traversal has missing guards | SecureMSM21300/21500; SPU tags alias | Requires matched modules; do not replace SCM/secure-world firmware |
| Touch/fingerprint | Exact product drivers, panel notifiers and calibration remain firmware-specific | Current official tags and narrow product-matched fixes only | Unresolved; reject foreign-product OPlus/Lineage feature imports |
| USB/Type-C | QCOM core/glue and recovery/normal module copies already have qualified .114 adaptations | No new narrowly applicable module fix established here | Unresolved; no speculative USB/DT changes |
| UFS/storage/ICE | Boot/FBE-critical stock QCOM/OPlus/SCM provider closure | No new matched storage-module series established here | Unresolved; preserve working storage and rollback paths |
| Power/thermal | Product-specific charger/thermal/clock/PM contracts | Only subsystem-specific proven PM fixes above | Requires firmware or userspace update; no foreign calibration or blanket OPlus update |
| Memory/DMA/IOMMU/synchronization | Existing speculative fence race candidate; prealloc stack-symbol buffer contract reviewed below | MM17600/17700; focused prealloc fix | Requires matched modules; no entire Synx/IOMMU generation migration |

OPlus-specific drivers are a cross-cutting constraint for all15 groups.
Foreign-product source drops identified in the old Lineage history remain
unsuitable as wholesale Canoe updates. The existing KernelSU/SUSFS, watchdog,
module trust, KMI and released boot-only architecture are preserved.

## Revalidation of the original forty candidates

The [40-row ledger](coloros17-candidate-revalidation.tsv) records exact SHAs,
affected paths/functions, current functional evidence, prerequisites and
source-versus-shipping-binary status. This audit examined changed code, not
just kernel versions or historical clean-apply labels.

- 28 source-level missing fixes have forward-applicable upstream hunks.
- 5 other missing source fixes need adaptation/dependency resolution.
- 6 exact later-generation paths are absent: passthrough peer callback,
  two later datapath helpers, DT bdfname formatting, CRM worker_ctx, and the
  newer HFI IRQ implementation.
- 1 camera session/lifetime item has a materially different coarse-lock
  implementation and remains investigation, not a proven vulnerability.
- No complete reverse patch check establishes an already-present fix.

These are source findings. None alone proves that a shipping .102 module is
vulnerable or unpatched. Separate binary analysis, active configuration and
build/provider evidence are required; no binary is marked fixed here.

The new project's classification vocabulary is exactly: `Compatible backport`,
`Requires matched modules`, `Requires matching kernel modifications`,
`Requires firmware or userspace update`, `Already applied`, `Not applicable`,
and `Unresolved`. The forty source-review rows currently comprise 24
`Requires matched modules`, 4 `Requires firmware or userspace update`, 6
`Not applicable`, and 6 `Unresolved`. Zero rows are approved as a deployed
`Compatible backport` or `Already applied` by this source-only review.
`Not applicable` here is limited to the six exact absent later-generation
source paths; it does not clear the shipping binary or all related defect
classes. The separate technical `source_status` and
`shipping_binary_status=NOT_PROVEN_BY_SOURCE_AUDIT` fields are retained.

Notable corrections to the old audit include CNSS `fada9b3d2ba2`: current
`cnss_get_bdf_file_name()` has no `bdfname_dt` branch. Adding a cosmetic `%s`
to unrelated constant strings would not backport that fix. Similarly, a
missing later datapath helper is not permission to import its architecture.

### Stage-A review: CNSS GPIO ownership

Original `ed87beed64bafbbfdc2f6f087132753c7fa00077` adds failed-request and
post-free invalidations. Its message is not a complete description of its
preconditions. The actual parent already initializes three SOL GPIO fields
to `-EINVAL` immediately after allocation and invalidates device SOL GPIO
after free/unwind/deinit. The current OnePlus source lacks these safeguards.
Therefore copying the forward-applicable hunks alone is not accepted as a
complete ownership repair.

Both parent and current `cnss_get_wlan_tsf_gpio_info()` initialize TSF GPIO
and IRQ to `-EINVAL` before DT lookup. This is not an initialization added by
ed87. Runtime TSF registration calls cnss_wlan_tsf_init; unregister calls
cnss_wlan_tsf_deinit unconditionally. GPIO-request and IRQ-request failures,
post-free invalidation, repeated unregister and any legitimate retry all need
an explicit ownership/unwind model. No incompatible struct change or feature
disable is authorized merely to simplify this review.

The exact parent snapshot and full patch are preserved in the reference
cache. A local adaptation must clearly identify retained upstream behavior
and any initialization/cleanup prerequisites; it must not claim an unchanged
original cherry-pick if additional source changes are needed.

### Stage-A review: RMNET ingress bounds

Original `ea47e85e97b72674a30e5846975c5a1eb04c6fde` touches only
`core/rmnet_descriptor.c`, `core/rmnet_handlers.c`, and `core/rmnet_map_data.c`.
At the clean audit baseline, source has unchecked header/packet-length derivations,16-bit GSO
subtractions and single-pass NLO delivery in both linear and fragmented paths.
The fix validates all NLOs before delivering any packet, checks cumulative
payload bounds and rejects malformed MAP/header lengths.

Public headers/exports are not changed by this patch, but that alone is not
private-ABI proof. The candidate must model all retained RMNET consumers and
the actual stock .102 provider. A working source patch does not establish
whether the shipping provider already implements these checks. The separate
PM-notifier SRCU change is not silently bundled into this bounds-only group.

Subsequent integration, not part of the original read-only source scan:

- `4ee253465786329a1156de7aad9000e9452a841c` preserves the actual stock
  `system_wq` import at the single aggregation-work scheduling site. The
  unchanged-source build otherwise gained a non-permitted
  `system_percpu_wq` import from current common headers; the narrow
  adaptation passed the compiled kernel import-policy gate without KMI
  list, signature, CRC or kernel-policy changes. The independent
  [private boundary/control review](coloros17-rmnet-private-abi.md) records
  all 25 callback KCFI checks and actual layout/ownership evidence.
- `084e43e044d572f30bec18ac3f0523d57d1c1351` preserves the individual
  official `ea47e85e...` bounds fix with provenance. Focused run05 compiled,
  linked and passed modpost; 140 exports, 156 versions, 155 actual ELF
  imports, 56 retained stock-consumer edges and compiled import policy
  passed. Its `rmnet_core.ko` SHA256 is
  `42a79a8f60c233a1c4ea3efeb35e19091d83f2c8d6c2a4320104b8e118e7aa7b`.
- The official patch alone is **not** labeled security-complete. The
  [independent bounds review](coloros17-stage-a-binary-review.md) found
  residual packet-count wrap, zero-count and earlier header-availability
  issues that need separate review/fixes and a new component qualification.
  Run05 is a focused build/provider check only, not the final full module
  oracle, production image, runtime PASS or deployment. No modified driver
  has been packaged, installed or flashed.

The forty-row ledger preserves the original clean-baseline apply checks;
its RMNET `source_status`/action/build fields additionally record this
subsequent isolated backport. The stock-binary source-audit status is not
changed into a deployed-fix claim, and the `Requires matched modules`
classification remains applicable.

Final isolated RMNET group after follow-up review:

- Local residual commit `b7c709d8cd1381837afd97f170af6c5b3190a078`
  widens only local packet counters, excludes zero-count descriptor GRO,
  guards early MAP/next/coalescing reads, and enforces the documented
  first-fragment legacy frame contract without pulling or linearization.
  Public/private layouts, callback contracts, common-kernel code and
  configuration remain unchanged. Unsupported mixed legacy layouts fail
  closed; normal/tethering/USB traffic still needs physical validation.
- Final run07 `rmnet_core.ko` SHA256:
  `7fa0bfcf5d79443156687c245ea3e39b995f39e1b21d1f28370fb1dac3d54191`.
  Compilation/link/modpost, all 15 independent private-layout comparisons
  and 25 callback KCFI checks pass. Full exact-population model replaces
  only `vendor_dlkm/lib/modules/rmnet_core.ko`, retaining all other stock
  modules and load metadata; control SDK dependencies are not deployed.
- Full static hybrid oracle: 1,070 modules, 1,032 active compatible,
  38 dormant, 60,701 hard imports, 61,777 matching CRC records, zero missing
  providers/CRC disagreements/namespace failures/active blockers. Runtime
  provider validation checks 54,785 vmlinux-first edges with zero errors;
  actual compiled import policy checks 54,986 unsigned edges with zero
  failures/protected collisions. All 3,220 ambiguity records remain
  unchanged; 56 RMNET consumer edges have one actual provider.
- These are static component/package-population results, not a new whole
  kernel build, signed/AVB DLKM image, runtime networking test or published
  release. No flashable DLKM partition image was created or flashed. The change requires
  matched replacement-module deployment; an unchanged stock DLKM continues
  running the unchanged stock driver despite these source fixes.

### Focused prealloc control-provider review

Same-family platform tip `2a3eaaee9851de11fbce570ce3b40b3b062d2327` has a
standalone candidate outside the old40:
`e6591422348dc7079b17b478b96d33d87f78c2c0`,
`cnss_prealloc: Increase symbol buffer size to prevent overflow`,
Karthik Kantamneni,2026-02-11, CR4436869,
Change-Id `I8725d29e35fe65595ebe598fe24524ff72be7674`.

It changes only `CNSS_SYMBOL_NAME_LEN` from128 to `KSYM_SYMBOL_LEN`, already
provided by the included kallsyms header. Current
cnss_record_stack_trace calls sprint_symbol directly into that array;
sprint_symbol requires the kernel-sized buffer. No additional source
dependency, public API or firmware change is needed for this one-line fix.
The altered allocation-tracking structure is module-private, but its users
and compiled configuration still must be checked.

Runtime applicability is conditional on `CONFIG_CNSS_PREALLOC_DEBUG_LEAK`.
The Canoe GKI fragment does not enable it, and Kconfig gives no default;
actual DDK defines and shipping code are decisive. Do not enable the feature
to manufacture applicability. If tracking is disabled, this can be described
only as inactive-path source hardening, not protection newly delivered to
the running phone. The larger same-family prealloc changes are pool tuning,
statistics/sysfs and debug features; the newer16.2.6 train additionally adds
SKB-pool APIs. None is selected wholesale to create an update count.

## Other new same-family endpoint changes

- WLAN10900 `77095667660c9eaeafd58083065722a208c2247e` fixes the P2P
  attribute loop to require a three-byte TLV header and check header+payload
  length before advancing. The current function still has the old checks,
  even though its remaining-length variable is wider than the subject's
  uint8_t wording. This is another source-level parser candidate, not yet
  stock-binary or deployed-fix evidence.
- Audio12500 `2bc34f6d8b61628135c9895a9f50e64c7be05b40` adds a
  `qcom,sleep_vote_disable` DT property for bourtzi/ravelin ADSP variants.
  It is not evidence of a required Canoe change. Do not modify DT/firmware or
  add unrelated audio state merely to adopt a newer endpoint.
- Camera15300 includes `33aa6410967d8308b97678ba6488f1ff915a43c4`, fixing
  a stale fence-row pointer after the final put. The current older function
  additionally dereferences the fence for a debug message after put, unlike
  the immediate upstream parent, which snapshots values first. Therefore
  the newest small patch alone is not a complete lifetime port. Inspect its
  full dependency chain and both dereference/row-reclamation paths before
  selecting it. No camera module is declared fixed by this audit.

## Access, isolation and next gate

Ref advertisements and selected original patch/blob objects were obtained
from official OnePlus/CodeLinaro sources. Downloads stayed in independent bare
repositories under `out/coloros17-driver-audit-reference/`; the existing
production remotes/refs were not updated. A partial-clone diagnostic initially
attempted unavailable lazy-object retrieval; it was stopped, and further
missing objects were fetched explicitly only into the isolated cache.
The HFI follow-up patch was fetched from the official Lineage repository only
to confirm its absent-generation source path, not as a build dependency.

No build, KMI/ABI PASS, replacement-module deployment, release, flash or
physical regression result is claimed here. Existing baseline gates do not
qualify new DLKM bytes. Proceed only with a bounded, coherent source series,
the exact provider/dependency closure, fresh production/module builds,
stock-consumer/private-ABI/signing validation, explicit packaging scope and
separate device authorization. Preserve every original rollback artifact.

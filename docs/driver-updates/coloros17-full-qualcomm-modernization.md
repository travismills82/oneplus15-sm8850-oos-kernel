# ColorOS 17.0.0.102 full Qualcomm modernization — experimental integration

Investigation date: 2026-10-30. OnePlus 15 / SM8850 / Kaanapali only.
**Not a release, not authorized for flashing, and not a claim that all public
Qualcomm changes or shipping binary vulnerabilities have been resolved.**
The production branch, qualified boot image, stock partitions and physical
qualification evidence remain unchanged. This effort continues source/build
work independently of the unresolved authenticated-DLKM deployment path.

## 1. Preserved baseline and source state

| Item | Exact identity |
|---|---|
| Firmware | `CPH2745_17.0.0.102(EX01)`; framework Android17/SDK37, vendor Android16/SDK36 |
| Production source/main | `efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c` |
| Qualified implementation-equivalent source | `10b349653c7b3a3a1e55d2fd357ed7220190c1b9` |
| Common-kernel tree | `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22` |
| Qualified kernel | `6.12.114-android16-6-o-gb6bd4a40410d-4k`, KMI6 |
| Continued experimental branch | `experiment/coloros17-sm8850-dlkm-updates` |
| Starting experimental HEAD | `0c4d5b23ab7ca52462fd40cb1c9ecb02438a0fe0` |
| Driver-fix source point | `e65991a0500c64fedacfc9a1e540c952d36c98e2` |
| Subsequent build compatibility | `566ab26631116` typed WLAN selection; `4237c3765dca9` exact manifest display inputs |
| Final implementation HEAD | `09decca5f42a2742401cacbcd1a1d4a6c0fd67b3` (subsequent commits may be tools/docs only) |
| Qualified boot SHA256 | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| Qualified Image SHA256 | `8ebe51e2a565a55886245793f572661e87c1a40bf5016bb79b84855370242456` |
| Qualified Module.symvers SHA256 | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Qualified common config SHA256 | `e5495ab30241f869f12b3fc2ff3590886150fb464c736975c8054303a9e9981a` |

The current experiment changes vendor sources, host verification/build helpers
and documentation, not common implementation/configuration, KMI lists, STG,
KernelSU/SUSFS, watchdog behavior, DT, firmware or module-loader policy. All
preserved upstream/ACK history remains untouched. No history is squashed,
reset, amended or force-pushed. The earlier untracked AVB report and unrelated
untracked/toolchain data are preserved rather than discarded.

The qualified source/build/artifact provenance is detailed in
[the baseline](coloros17-qualified-baseline.md). A newer Git HEAD is not a
claim that the physically qualified Image was rebuilt from it.

## 2. Upstreams, current revisions and review scope

[The 27-row source-reference inventory](coloros17-upstream-refs.tsv) records
24 CodeLinaro repositories plus three official OnePlus references, with
canonical URL, previous/current peeled tags, exact SHA and different-train
alternatives. All three requested vendor namespaces were investigated.
Git access succeeded even where the web frontend was inaccessible. Lineage
history is a provenance locator only, never a target/build dependency.

Highest inspected same-family references include WLAN10900, Graphics10300,
Display/MM17700, Camera15300, Audio12500, Video12600, BTFM08400 and
LA.VENDOR.16.2.0.r1-21500. Many newer numeric tags alias already inspected
code. Different r5/r6,16.2.6, Graphics16.1 and Display6.11 trains were
enumerated but are not presumed ColorOS-compatible. Exact per-repository
tips and firmware/dependency decisions are in the inventory and subsystem
ledgers, not inferred from the marketing OS version.

No exact ColorOS .102 source archive/manifest was available or advertised.
All 1,070 shipping binaries retain `EXACT_REVISION_NOT_VERIFIED` provenance;
source names, tags, version strings and CRC matches do not establish their
complete implementation. Select replacements below have much stronger
unchanged-control instruction/interface evidence, not invented source SHAs.

The original [40-candidate functional review](coloros17-candidate-revalidation.tsv)
is preserved as a pre-integration snapshot. This continuation additionally
reviews the official camera `408f334b` prerequisite, `33aa6410` follow-up
and WLAN `77095667` endpoint: **43 named upstream bug-fix candidates**.
This count is focused functional review, not every commit in every release
train, every CVE, or a claim of exhaustive proprietary-firmware coverage.

## 3. Integrated changes and logical commits

Since the accepted `0c4d5b23ab7ca` point, 29 isolated implementation/test/input
commits preserve 19 exact original patches plus one adapted upstream fix.
Eight additional local commits provide necessary coherent guards, explicit
stock-provider behavior or a style-only follow-up; these are not eight newly
claimed CVEs. One separate commit restores 57 exact manifest-pinned missing
display source files. The pre-existing three-commit RMNET series remains intact.

| Group | Original fix | Local commit | Source/build/deployment distinction |
|---|---|---|---|
| WLAN | `69a2230a0e2888797b5ad9afb898e7955f438a9a` | `1f55863ac696b` | MLIE span; final module compiled, quarantined |
| WLAN | `f0eb2ec58ebbc3f5b1c595399023d674d435bc6f` | `0a7d4d80444d6` | HE bound; coherent local follow-up required/present |
| WLAN | `7bf5821c503b536212ea8993bb854480a494e28c` | `b9dd611f14787` | MLO LL_STATS guard plus local STA-only correction; compiled, quarantined |
| WLAN | `11a03259c48fa54f6bc67a39309740e62b776776` | `1f6dd0d1aae3d` | Completion vdev reference; compiled, quarantined |
| WLAN | `b0af9d5ebef64de3178f49dcbddf54ce2b53e245` | `4fd9bfcf1136d` | Runtime-PM disable contract; compiled, quarantined |
| WLAN | `77095667660c9eaeafd58083065722a208c2247e` | `e65991a0500c6` | P2P TLV span; compiled + sanitizer-tested, quarantined |
| SecureMSM | `340fcf5a1373745e4949c60488a8ac9721770179` | `ff1f9c0dda81e` | QCE + QCrypto SG guards; compiled and stock-boundary qualified |
| DSP | `89e310d94c751816fbc3ec008c8dbe9e6e1a6017` | `9905eabc27340` | Minimal registration-state port; compiled, quarantined |
| GPU | `1d15bd800fe5dad77ba5a4a4571e1478bae3a52e` | `6285ba23b5c97` | VMA fault offset; compiled, quarantined |
| Camera | `abb98ee3b27b9df161a490a15ccdd3c22ab712c0` | `93c8986ac3f14` | JPEG manager/context lock order; compiled, quarantined |
| Camera | `afacc37ba1c8ff3eeb9cdbe02be4fe87a5b52e32` | `ba59514314ee9` | IFE capacity; compiled, quarantined |
| Camera | `047de0202db8b28c0cfa99ed11702691b86e883e` | `409e9b3d9196c` | ICP dump room; compiled, quarantined |
| Camera | `d58c17180c41de236c408313b1a96e348557f7b5` | `272c2320d5662` | OPE size check; dormant source, NOT compiled active Canoe protection |
| Camera | `668f72743a751668a259b1dc488dfa2f6e6b00ff` | `640d1bff2818c` | Dual-config port validation; compiled, quarantined |
| Camera | `ce2c54db852c0171cc6256cc78be42a1cdc3b1db` | `2ff82d357f07c` | Stale request-map clearing; compiled, quarantined |
| Camera | `11a7a73b3b73242ec97343edd41de422015613ff` | `3a04135a392d5` | FCG actual blob span; compiled, quarantined |
| Camera | `408f334b69bb1cd08b973dace1908e0b0a98b430` | `8c8aadeb9662f` | DMA fence owner/identity prerequisite; internal callers migrated |
| Camera | `33aa6410967d8308b97678ba6488f1ff915a43c4` | `e3e7b8821eabb` | Last-put row invalidation; compiled with separate residual correction |
| Video | `cfed001431a2e623154fde329a31e9fa2a534b31` | `7f2d88b48379f` | SSR validation before state mutation; compiled, quarantined |
| Audio | `4c86a9329b871a685120330d4bb42aed5f9f8b6f` | `cda06aea6ef78` | Timeout-only DSP clock compensation; compiled and stock-boundary qualified |

Local commits: QCrypto stock queue `fd36db8b15813`; redundant QCE IRQ-enable
removal `ddd4de0324cb9`; real HE/VHT/short-reassociation bounds
`1d91a86fac0d5`; camera monitor/owned-reference residual `fbdf1d7c671b8`;
dormant OPE copy/error residual `e41f8fbecf6d7`; style-only camera diagnostic
`92ff219b2b6df`. Exact original authors, messages and relative patch IDs were
retained. Canonical repository corrections for five trailer path spellings
are documented in the [networking ledger](coloros17-full-networking-wlan.md)
without rewriting commits.

Local `09decca5f42a2` restricts the MLO progress check to STA mode: SAP
allocates `ap_ctx`, not the helper's dereferenced `sta_ctx`. The original
Qualcomm change stays separate and the AP NULL path is not introduced.
Source-compatibility commit `566ab26631116` replaces an invalid version-number
guess with two correctly typed monitor callbacks selected by the actual
cfg80211 field type. It introduces no pointer cast or fallback for unknown
signatures and is not permission to mix the new callback with stock cfg80211.
Build-input restoration `4237c3765dca9` recovers exactly the 57 original files
from manifest `5ab2a689...`, subtree `2b988ef69185701366472f18339576ac4b3ecf98`;
all staged blob IDs, modes and original line-ending bytes match. These are
neither new upstream driver fixes nor proof of shipping display parity.

Host tooling/test infrastructure is isolated in `58162bcc9c67d`; it changes
no kernel/module implementation, configuration or ABI reference. The final
documentation commit is separately discoverable with `git log` and does not
change the held implementation/build identity `09decca5`.

## 4. Subsystem outcomes and private boundary gates

| Subsystem | Result / remaining work |
|---|---|
| RMNET | Existing ingress hardening retained and combined oracle PASS; separate PM/SRCU fix held for coherent teardown/RTNL/resource proof, not because AVB is blocked. |
| IPA/data acceleration | Mapping-aperture restriction needs actual firmware/QMI contract; unmodified SDK already differs on USB exports. Retain stock. |
| WLAN | Six source fixes + coherent local bounds/AP guard. Final full matched-source CC/LD/modpost/module build PASS with genuine SDKs. Typed selection naturally uses the current three-argument ABI; stock cfg80211 expects two. Quarantined; no unsafe cast/header fabrication or stock-provider substitution. |
| Bluetooth | Same-family endpoint aliases prior code; stock GKI/vendor arrangement preserved. No built-in/module-shadow architecture. |
| SecureMSM | QCE/QCrypto compile, retained stock edges, natural CRCs, eight inspected private types per module and mapped KCFI unchanged. No secure-world/firmware claim. |
| SPU | Refreshed endpoint aliases prior code; no independent validated candidate selected. Stock retained. |
| GPU/KGSL | Patched compile PASS; exports/imports/KCFI unchanged vs control; three critical layouts identical. Unchanged control has 11 stock-provider import failures and two retained-consumer failures. Do not package. |
| Camera | Patched compile PASS, public imports/exports unchanged vs control, six inspected layouts identical. One nonexported helper signature intentionally changes with all callers migrated. Stock control has 42 camera_extension edge failures + two socinfo imports; matched extension rebuild resolves its pair-local 21 imports but not socinfo/HAL/calibration provenance. Quarantined. |
| Video/HFI | Patched compile PASS; baseline has two socinfo + six synx import CRC failures and a workqueue-policy failure. SSR/PM lifetime follow-ups held for actual power/teardown proof. Quarantined. |
| Display/DRM | Exact 57 manifest files recovered after initial missing-input failure. Full restored control CC/LD/modpost PASS; shipping audit FAIL: 10 provider CRC mismatches (HFI1/socinfo3/Synx6), a workqueue policy failure, 28 export and 17 KCFI differences. Quarantined, no behavior patch; rotator inactive. |
| Audio | Full WCD module control/candidate compile PASS; only clock prepare changes, eight private layouts and 27 mapped KCFI words unchanged; 11 stock codec edges pass. Genuine PRM/GPR SDK producers are not deployed. |
| DSP/FastRPC | State guard compiles, only one function changes vs control; 12 inspected layouts/161 KCFI words unchanged. Pre-existing shipping feature divergence and forbidden `system_percpu_wq` import remain. Quarantined. |
| USB/DWC3 | No additional proven compatible module change selected. Preserve qualified .114 DWC3/private ABI adaptation; never assume CRCs prove the old/new 1488/1576-byte private layout compatible. |
| UFS/ICE/storage | No matched new module security series established; FBE/SCM/keyslots/DT remain untouched. |
| Power/thermal/clocks/regulators/interrupts | Only reviewed local QCE IRQ and audio clock behavior above; no foreign DT, calibration or broader PM migration. |
| Touch/fingerprint/OPlus | Official source/tag inventory retained; product-specific/private/HAL parity not proven for a replacement. Stock retained. |
| Memory/DMA/IOMMU/sync | Unchanged sync-fence control builds. Proposed upstream lock patch would recursively acquire a fence lock and leaves concurrent-commit TOCTOU; not imported verbatim. Genuine Synx/hw-fence SDKs do not qualify replacement modules. |

Detailed exact symbols, expected/current CRCs, private layouts, KCFI tags,
control/candidate evidence and rejected dependencies are in:
[networking/WLAN](coloros17-full-networking-wlan.md),
[SecureMSM/DSP/audio](coloros17-full-security-dsp-audio.md),
[multimedia](coloros17-full-multimedia.md), and the preserved
[source comparison](coloros17-upstream-comparison.md).

No blanket claim is made about private types not inspected. Zero new drift
versus an incompatible unchanged control is useful evidence, not stock PASS.
No throughput, latency, power or other performance improvement has been
quantified; source correctness fixes are not advertised as measured gains.

## 5. Exact integrated population and real-provider oracle

The [1,070-row inventory](coloros17-module-inventory.md) remains authoritative:
103 system_dlkm, 461 vendor_dlkm, 506 vendor_boot files; 702 distinct canonical
names, not 1,070 simultaneous drivers. The accepted experimental population
replaces **only four vendor_dlkm files**: RMNET core, QCE, QCrypto and WCD.
Their exact paths/hashes are in the security/audio ledger and private manifest.
The [four-row inventory overlay](coloros17-modernization-module-overrides.tsv)
records their exact partition-qualified paths, canonical names, final stripped
hashes, source commits and separate static/packaging/device gates. Apply this
overlay to the immutable 1,070-row stock inventory; the other 1,066 binary
records remain unchanged, rather than rewriting their original provenance.
None has a competing system/vendor-boot copy. Every other 1,066 binary files
and all captured load/dependency/alias/softdep/blocklist metadata remain stock.

Fresh combined oracle, not a sum of subsystem results:

```text
Modules checked:          1070
Active compatible:        1032
Dormant:                    38
Hard imports:       60701/60701
CRC records:        61777/61777
Missing providers:           0
CRC mismatches:              0
Namespace failures:          0
Active blockers:             0
```

Actual vmlinux-first validation covers 54,785 edges; compiled GKI policy
covers all 54,986 unsigned-import edges with zero failures. All 3,220 pre-existing
ambiguous edge/provider records are unchanged. The four replacement providers
have 89 distinct existing consumer edges (RMNET56/QCE22/WCD11/QCrypto0),
including rebuilt QCrypto's preserved imports from QCE.
No stock provider is substituted to hide a mismatching rebuilt one. SDK-only
camera/Synx/IPA/PRM/etc. outputs are never silently installed.

Evidence:
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/security/integrated-oracle-final-filesystem-run02/`.
This final run consumes the post-strip, post-AVB extracted candidate filesystem;
the preceding run01 is separately preserved, not substituted for final payload validation.
Compilation and this oracle do not establish physical module initialization,
firmware behavior or a working verified-boot deployment path.

## 6. Fresh build and test gates

Per-module normal DDK config/makefile generation and isolated source snapshots
use the exact qualified common outputs, genuine generated producer SDKs and
normal modpost. No CRC table is fabricated, warning ignored, signature check
disabled or module force-loaded. Controls are captured before edits; manifests
separate generated headers, source hashes, dependency producers and outputs.

Important target boundary: this checkout's
`soc-repo/kleaf-scripts/techpack_modules.bzl` defines techpack registration
as a no-op. Its generic production distribution registers SoC modules plus
13 explicit OPlus modules; it must **not** be credited with compiling the
changed WLAN/crypto/camera/etc. techpacks merely because that parent target
succeeds. Their separate exact-target/configuration/Kbuild/modpost evidence
is required and recorded here. No unrelated driver is silently enabled to
make the distribution appear comprehensive.

Fresh common/production/Canoe ABI attempts and their final outcomes are tracked
in `coloros17-full-build-ledger.md`; they must not be inferred PASS from an
earlier artifact. The enforced GKI comparison baseline remains **FAIL/exit4,
38 reviewed differences**, not PASS. No ABI reference update is authorized
or performed. Common implementation/configuration/KMI inputs remain identical
to the qualified baseline; fresh output identity is independently checked.

The fresh `//common:kernel_aarch64` build completed successfully in1756.436s,
including actual strict-KMI and symbol-list-violations markers. Its SCM is
recorded separately from final vendor-only commits; common contents match.
Normal common ABI generation then legitimately rebuilt for final `09decca5`
and completed PASS in1554.943s. Fresh config, Module.symvers and vmlinux-only
export records have exactly the frozen hashes above. Both full and filtered
generated STG files are byte-identical to the frozen five-CVE baseline.

The fresh **enforced** common comparison completed **FAIL/exit4**. All five
reports (plain, flat, short, viz and small) are byte-identical to the frozen
five-CVE reports: exactly the same 38 reviewed differences, zero new delta.
This remains FAIL, not a waived or relabeled PASS. Eight BTF type-ID
deduplication warnings match the frozen production log byte-for-byte; no new
compiler warning is established there, and no STG parse failure is hidden.

Normal `//soc-repo:canoe_perf_dist` completed PASS in1207.322s at final
`09decca5`. It retains eight dependency warnings over six distinct modules:
charger-ulog-glink, leds-qcom-flash, msm_memshare, qcom-hv-haptics,
qcom_q6v5 and wcd_usbss_i2c. Their exact eight edges match the frozen generic
production log: zero new/removed warnings. These generic images are not
the stock-population four-provider package and are not deployment candidates.
Canoe ABI generation separately completed **PASS** in2545.936s after its
required `with_vmlinux` kernel/module build context. This target has no
maintained enforced device-ABI reference/diff; it is generation PASS, not
an enforced stock-device ABI comparison. Modified techpack contracts remain
covered by the individual and complete hybrid/provider audits above.
Both full and filtered Canoe STGs are byte-identical to the frozen five-CVE
baseline. A final explicit normal common-kernel/strict-KMI invocation at
held `09decca5` completed PASS in2.590s using normal action caching, closing
the earlier SCM-state distinction without a flag override or borrowed PASS.

| Final offline gate | Actual result |
|---|---|
| Common kernel / strict KMI | PASS, final exact source invocation |
| Common ABI generation | PASS; full/filtered STG identical baseline |
| Enforced common comparison | FAIL/exit4; same38 reviewed differences, zero new |
| Canoe production distribution | PASS; eight unchanged baseline dependency warnings |
| Canoe ABI generation | PASS; full/filtered STG identical baseline; not an enforced device diff |
| Four-provider extracted-image oracle | PASS,1070 locations/1032 active/38 dormant |
| Production provider/import policy | PASS for exact four-provider population |
| Private layouts/KCFI | PASS for selected four; failed independent controls quarantined |
| ext4/readback/SELinux/hashtree/FEC | PASS for exact offline image |
| Actual phone LP/snapshot validation | BLOCKED, metadata/device unavailable |
| Authenticated parent AVB deployment | BLOCKED, device-specific authority/path unproven |
| Physical testing of new modules | NOT TESTED |

Fresh host/static repeat: all117 tests across seven suites pass (WLAN8,
multimedia25, security/audio5, LP capacity19, build infrastructure20,
inventory17 and RMNET23). Negative mutation controls are included within
those groups, not inflated into extra tests. None is a physical-device PASS.

Functional patch whitespace/checkpatch checks are separate from the exact
57-file restoration. The latter intentionally preserves upstream CRLF,
trailing whitespace and missing-final-newline bytes in eight files; a full
restoration-range `git diff --check` is not claimed PASS. No global whitespace
rule is weakened or upstream source silently normalized to hide that fact.

Focused tests: eight WLAN groups (including actual C ASan/UBSan, typed callback
selection and negative controls), 25 multimedia range/source-contract tests, five security/audio
groups with five negative mutation controls, and 19 synthetic liblp format/
failure tests. Synthetic models/source contracts are not kernel race or
hardware tests. Build-helper tests and existing RMNET/inventory tests are
reported separately, not relabeled physical validation.

## 7. Images, metadata, capacity and authentication

No built-in kernel change is needed for the four qualified replacement modules.
Use the exact frozen qualified boot, not a rebuilt image made merely to change
the revision string. Stock system_dlkm, vendor_boot, init_boot, DTBO, userdata,
metadata and firmware are not replacement artifacts for this tranche.

Offline vendor_dlkm filesystem construction uses the exact hybrid and stock
ownership, modes, paths, SELinux labels and all nonreplacement files. It must
be a fresh metadata-aware ext4 build: stock uses `shared_blocks`, has only
112 free 4-KiB blocks, and logical module bytes already exceed filesystem
data capacity by 1,282,976 bytes. In-place debugfs unlink/write on shared
extents is not an acceptable stager. Build results, extraction/readback
hashes, filesystem checks and partition-local AVB verification are recorded
in the packaging ledger; successful image generation is not deployment PASS.

The fresh offline image **was built successfully** within the original size:
37,076 used blocks and 111 free, versus stock 37,075/112. Read-only e2fsck,
all 471 file readbacks, and all 476 UID/GID/mode/SELinux identities pass;
only the exact four module files differ. All module metadata remains byte-
identical. Partition-local SHA256 hashtree verification and independent FEC
re-encoding pass. Filesystem timestamps/lifetime-write counters are documented
reconstruction differences, not falsely claimed identical.

| Experimental payload | Location / SHA256 | Scope |
|---|---|---|
| `vendor_dlkm.img` | `security/offline-vendor-dlkm-run06/vendor_dlkm.img` below the private evidence root; `b8a5385cb563188640395b4117681b782f8e3a97fcd914d8ce6a34af0e6e62db` | New offline four-module filesystem; NOT authenticated for the device parent |
| `boot.img` | Preserved qualified `images/qualified-boot.img` in rollback directory; `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` | Existing exact payload, not rebuilt or modified |
| `system_dlkm`, `vendor_boot`, `init_boot` | No replacement candidate produced | Remain stock |

The [complete offline package report](coloros17-four-provider-offline-package.md)
contains all four stripped module hashes, original filesystem metadata,
build/failed-attempt evidence, stock and new AVB root digests, FEC verification
and the final extracted-filesystem oracle. It does not call an unsigned local
footer OEM authentication.

The stock-size envelope is 152,317,952 data bytes and 154,849,280 total image
bytes. This is **not** a substituted super capacity check. Actual LP metadata
and a fresh snapshot-state capture have not been supplied/found, so dynamic
group limits, allocatable extents, inactive-slot layout and live Virtual A/B
state remain BLOCKED/UNKNOWN. Archived `super_size` and raw DLKM lengths
cannot prove them. No unrelated partition is resized.

`tools/driver-updates/validate-coloros17-dlkm-capacity.py` parses/checks real
liblp primary/backup geometry, selected metadata, SHA256 checksums, table
boundaries, unique extent ownership, physical overlap/ranges and group limits.
It checks candidate fit without resize, rejects sparse/unbacked-zero targets,
and gates snapshot state independently. With no actual metadata it fails
closed; the 19 passing synthetic tests are not real-device capacity proof.

Additional format validation used the real AOSP `lpmake` **synthetic** super
produced by normal Canoe validation. After official `simg2img` expansion,
the parser verified both geometry/metadata copies, checksums and allocations
and recovered the exact lpmake parameters: super570,425,344;
system_dlkm98,353,152; vendor_dlkm179,417,088; group limit566,231,040.
The four-provider image fits that synthetic vendor extent with zero deficit;
the tool still exits2 because snapshot state is unknown and deployment is
not authorized. Evidence is `capacity/synthetic-super-fixture-01/` under
the private archive. **None of these synthetic sizes is actual ColorOS LP
metadata or proof of live partition/snapshot compatibility.**

The preserved [AVB feasibility report](coloros17-vendor-dlkm-avb-feasibility.md)
still applies: stock top-level RSA4096 vbmeta directly authenticates the
vendor_dlkm hashtree descriptor; the partition-local footer is algorithm NONE.
A regenerated correct local hashtree/FEC/footer does not change the signed
parent descriptor. No OEM signing authority, enrolled custom-key arrangement
or device-specific authenticated developer path is established. Offline
images are **BLOCKED FOR PHYSICAL DEPLOYMENT**, not production-signed.
No vbmeta, rollback index, verification flag or device state is modified.

## 8. Security accounting and watchdog scope

These are source-level memory-safety/lifetime/correctness improvements, not
20 newly proven CVEs fixed on an untouched device. Shipping binaries remain
unmodified until an authorized candidate is actually loaded and tested.
No proprietary firmware, secure world or userspace vulnerability is declared
fixed by a kernel/module source patch alone.

The task's CVE-2026-80668 HOLD describes an older artifact: the accepted
baseline already contains the independently reviewed/physically tested
conntrack repair `633ac4e5f56b64aa9cf80d5813c6fac82cf43d82`, from official
`b8b09dc2bf35a00d4e0556b5d6308c7b917ebda2`, plus the later five-CVE series.
It remains present and is not credited to DLKM modernization. The preserved
validation reports record this supersession. No DLKM CVE-completeness claim
is made where binary applicability or firmware prerequisites remain unknown.

Existing boot-only watchdog filtering/protections remain unchanged. None of
these driver changes is claimed to solve `adci_thread`, `zram_comp` or
`hfi_core_dbg_cl` merely because it touches a related subsystem. Future tests
must distinguish deliberate idle waits from active SCM/QPACE/HFI stalls.

## 9. Rollback manifest and future physical procedure

The [21-row physical test plan](coloros17-modernization-physical-test-plan.md)
records all new subsystem results as NOT TESTED and defines stage-specific
artifact/preflight/readback/failure gates. It is not a flash authorization.

Independent preserved directory:
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-baseline-20261030/`.

| Payload | SHA256 |
|---|---|
| `images/qualified-boot.img` | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| `images/stock-system_dlkm.img` | `3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a` |
| `images/stock-vendor_dlkm.img` | `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47` |
| `images/stock-vendor_boot.img` | `d0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000` |

`baseline-sha256.txt` additionally seals the common config and Module.symvers.
Filesystem-only `*-verity.img` mounted views are not raw rollback payloads.
Source/toolchain/config/producer/build manifests remain alongside each private
candidate. Original physical reports are unchanged.

Before any separately authorized physical test:

1. Establish an authenticated device-specific AVB path and exact affected
   parent components without disabling verification or inventing OEM keys.
2. Capture firmware identity, active/inactive slot, raw LP metadata, dynamic
   group/extent limits, partition capacity and current snapshot/update state.
   Stop on pending/merging snapshots, ambiguous mappings or unknown capacity.
3. Rehash qualified rollback and exact candidate; verify filesystem, every
   packaged module/metadata/label, AVB descriptors and actual provider order.
4. Use fastbootd only if the reviewed dynamic-partition procedure requires
   it; never infer flash permission from mode availability. Back up/hash all
   specifically affected active-slot components first; preserve inactive slot.
5. Run staged experiments: RMNET alone; then QCE/QCrypto; then WCD clock
   timeout candidate. Do not begin with quarantined camera/GPU/WLAN/Video/DSP.
   Each stage requires the complete same source/stock dependency population,
   not ad-hoc file mixtures or a broader generic Canoe image.
6. After authorized scoped writes, perform full exact readback before boot;
   verify kernel/module identities and no load/CRC/namespace/signing errors.
7. Independently record boot, user0 RUNNING_UNLOCKED, CE/DE, KeyMint,
   TWRP PIN/mapper, F2FS+inlinecrypt; then RMNET/LTE/5G/voice, WLAN,
   Bluetooth, GPU/display, camera/photo/video, audio/microphone/calls,
   USB/OTG, charging/thermals, screen-off and USB-disconnected suspend/resume.
   Observe watchdog/error logs through at least 240 seconds and preferably
   10–15 minutes; retain pstore and all first-failure evidence.
8. On failure, restore only the reviewed affected raw components from this
   same firmware generation, read back/hash, and verify Android/FBE again.
   Never wipe userdata/metadata, change encryption policy, disable AVB,
   resize unrelated partitions or change slots blindly as a recovery shortcut.

Every physical test in this new modernization phase is **NOT TESTED**.
No device command, flash, reboot or manual hardware PASS is inferred here.

## 10. Recommendation

Continue staged development; do not promote a generic latest-tag or complete
public-source driver stack. Four selected replacement modules have a clean
combined stock contract, while several unchanged driver controls prove the
public source is not interchangeable with shipping .102 binaries. Those
concrete boundary failures including restored display, actual LP/snapshot
evidence and authenticated AVB deployment remain explicit blockers.

The experiment is **not ready for a controlled device flash** until the
remaining package/capacity/authentication gates are resolved. Source-level
integration, compilation and offline qualification are not stopped merely
because deployment authority is unresolved, but no incomplete gate is
converted to PASS. Do not push, merge, publish or flash this branch.

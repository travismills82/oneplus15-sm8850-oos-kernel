# ColorOS 17 full modernization: build infrastructure and fresh gates

Date: 2026-10-30. This is experimental work, not release qualification.
The qualified boot-only production payload and all stock firmware images are
preserved. No AVB policy, top-level VBMeta, device partition, or kernel source/
Kconfig was changed by these infrastructure tools.

Private build evidence:
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/build/`.
Other agents' component controls are under that evidence root's sibling
`security/` and `multimedia/` directories, not deployment artifacts.

## Immutable external-module SDK

The tools deliberately build modules against the physically qualified kernel
outputs, not a freshly inferred kernel ABI. This is a normal external-module
prebuilt-SDK build: generated release/config/version inputs come from that
kernel, not binary vermagic editing or manually generated CRCs.

| Qualified input | SHA256 / tree identity |
|---|---|
| Common source tree | `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22` |
| Image | `8ebe51e2a565a55886245793f572661e87c1a40bf5016bb79b84855370242456` |
| Module.symvers | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| modules_prepare archive | `28cd2f46c9aed946aaa4ee0729fea01791c13e0e554e932813acf6a37e2620dd` |
| Qualified build environment | `8c4e24504cadcde362a8b714c281c686459c863e685285cddb9b2000006c5243` |
| Actual vmlinux-only export records | `fa6d325e5e5ce232ca7622176829ce7c4bcf7cf7ea47070bf6f48b265da02efb` |
| Generated qualified localversion | `8c1bf64610fb6c40cad3b5236d33c243048834777601b8eb17dbbab6c86534e0` |

The latter four inputs and exact common tree are checked before each new
isolated run. Different common source requires a separately planned kernel
build; this helper must not conceal such drift. A full production validation
is separately being attempted below and is not implied by a module compile.

## Reusable tools

`tools/driver-updates/isolated-ddk-module-build.py` supports independent source
and preparation worktrees, an explicitly reviewed source HEAD, exact expected
module outputs, fresh private output directories and complete genuine SDK
producer tables. It never invokes Bazel or a kernel Image build itself.

Example, after normal target configuration/makefile preparation:

```sh
W=/path/to/reviewed/source-worktree
HEAD=$(git -C "$W" rev-parse HEAD)
python3 -B tools/driver-updates/isolated-ddk-module-build.py \
  --source-worktree "$W" --prepared-worktree "$W" \
  --expected-head "$HEAD" \
  --label //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc \
  --module-out cnss_prealloc.ko \
  --run-dir /private/new-control-run --jobs 2
```

`--plan-only` performs input checks without creating outputs. Source Kconfig/
generated `.config` is not edited. Source and build outputs use separate
directories; host-tool RPATH support stays inside the requested run directory.
An existing output directory, noncanonical label/path, unsupported source root,
common-tree drift, changed pinned SDK input, or unexpected module population
is rejected. No successful module is overwritten by the retry helper.

Each new run records copied source/build-definition hashes, generated flags,
normal generated-header bytes, SDK input hashes and actual compiler dependency
hashes where depfiles exist. Unresolved dependency names are reported, not
silently treated as verified. Generated Makefile/cflags path substitution is
normal external-build integration; no source declaration/CRC is rewritten.
Existing nonexistent `-I` alternatives may be harmless normal DDK search
directories; a missing forced include is a hard failure. Generated include
directories are not recursively copied: they can contain complete runfiles or
toolchain trees. Required generated files are frozen byte-for-byte; additional
headers actually consumed through directory searches are recorded from compiler
dependency lists.

The first patched WLAN run predates that directory-copy bound: its private
generated-input capture contains33,306 files (about3.7GiB), retained unchanged
because compilation uses those paths. A filename-only review found no `.pem`,
`.key`, `.p12` or `.pfx` entries. This is private build evidence, not a source or
release payload. Subsequent runs use the bounded capture behavior.

The default copy covers only the DDK package. For cross-package source such as
WLAN use `--freeze-source-tree vendor/qcom/opensource/wlan`; it copies the
entire requested source closure before compilation. Common headers remain
shared read-only under the fixed common-tree guard. Other dependency sources
must be frozen explicitly if they may be changed concurrently.

`resume-isolated-ddk-module.py` retries a failed control's already frozen source
with real additional SDK tables or a newly prepared normal generated header.
It preserves the original failed log/script/result, records a uniquely named
attempt, hashes its source/SDK/generated inputs and never reloads newly edited
live driver source. This is not permission to ignore modpost or CRC errors.

`locate-generated-sdk-providers.py` maps actual object undefined names to
existing unmodified producer Module.symvers files. It creates path inventories,
never new CRC tables. It distinguishes generated `__this_module` from a runtime
provider. SDK candidate provenance/configuration and every actual used export
still require stock/runtime validation; finding a path is not qualification.

`prove-ddk-preparation-closure.py` follows actual action/artifact producer edges
(including generated tree-file members), not the broader configured target
graph. It rejects KernelBuild/KernelModule/Image actions in the requested
preparation closure. Trusted generator definitions still need source review.

For example, normal preparation for the existing CNSS-prealloc module is:

```sh
set -euo pipefail
cd "$W/kernel_platform"
tools/bazel aquery --output=jsonproto --include_commandline=false \
  'deps(set(//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_config //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_makefiles))' \
  > "$E/actions.json"
python3 -B "$W/tools/driver-updates/prove-ddk-preparation-closure.py" \
  "$E/actions.json" \
  //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_config \
  //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_makefiles
# Continue only after the preceding command succeeds with result PASS.
tools/bazel build --jobs=4 \
  //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_config \
  //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_makefiles
```

Here `$E` must be a new private evidence directory created by the operator.
Do not substitute a module group for a real DDK-module label or run a rejected
preparation closure. Per-run control/candidate logs preserve the exact SDK
table paths; later shell retries do not overwrite original failure evidence.

Host-only infrastructure tests:20 passed. They exercise unsafe paths/labels,
source mutation detection, missing required generated headers, unchanged
header-byte capture, complete explicitly frozen sibling-tree mutation detection,
Kconfig/build-config variants, nonrecursive generated-directory handling and
kernel/module actions hidden behind dependency sets or tree artifacts. These
are tool tests, not driver/runtime tests. Actual compiler-dependency capture is
scoped to the built module's `.cmd` files: historical host-tool `.cmd` files in
the modules_prepare archive are not falsely labelled inputs compiled by the
new run. The historical wider capture is retained, not silently overwritten.
An unchanged generated flags file retains its timestamp during later retries;
rewriting identical bytes must not force unnecessary whole-driver recompiles.

## Normal preparation attempts

Only target configs, makefiles and specifically required generated headers
were requested; no full driver module or Image action occurred in these steps.

| Evidence | Requested normal inputs | Outcome |
|---|---|---|
| preparation-01 | GPU, camera, QCE50, QCrypto, FastRPC, WCD codec, active peach WLAN, CNSS2 configs/makefiles | PASS;16 targets;803 required closure actions;39.592s actual build |
| preparation-02 | camera banner, WLAN feature-grep header; audio PRM, video, display, sync-fence configs/makefiles | PASS;10 targets;704 required closure actions;23.894s |
| preparation-03 | OPlus osvelte, Synx; GPR, notifier, SPF, audio daemon, feedback configs/makefiles | PASS;14 targets;771 required closure actions;41.122s |
| preparation-04 | q6-PDR, sound-event, ADSP loader, IPClite, HW fence, CNSS NL/utils configs/makefiles | PASS;14 targets;773 required closure actions;39.671s |
| preparation-05 | guessed group `canoe_perf_mmrm_driver_config` | FAIL before execution: that group has no `_config`;0 processes |
| preparation-05-retry | actual module `canoe_perf_msm-mmrm_{config,makefiles}` | PASS; normal cached inputs;0 new actions |
| preparation-06 | OPlus camera-extension configs/makefiles | PASS;2 targets;635 required closure actions;22.760s |
| preparation-07 | HDCP/SMCInvoke, HFI core, external-display, touchpanel-notify configs/makefiles | PASS;10 targets;725 required closure actions;18.886s normal build |
| preparation-08 | SMEM mailbox configs/makefiles (three actual unresolved CNSS-utils SDK APIs) | PASS;2 targets;633 required closure actions;8.27s critical action path (1560.802s command elapsed includes waiting behind the ABI kernel build) |
| preparation-09 | WLAN firmware-service and CNSS IPC-QMI configs/makefiles | PASS;4 targets;657 required closure actions;21.181s; requested only after actual CNSS2 modpost demonstrated this closure |

The MMRM module output is `msm-mmrm.ko`, not the group label. The failed guess
remains preserved. A source/target name is not assumed from a module filename.
Config-generation warnings (including the existing WLAN IPA bool/tristate and
FEATURE_DIRECT_LINK redefinition warnings) remain in full logs.

## Original-control proof and SDK boundaries

Generic-builder smoke: unchanged CNSS compiled/link/modpost PASS; all7 exports,
28 version records,27 hard imports,29 allocated runtime sections and342
canonical relocations match exact stock. Its debug-bearing file hash differs
from earlier controls; code/runtime-section identity was independently checked.
The normal external build skips BTF generation when no vmlinux input is
supplied; this disclosed message is not a BTF success claim.

The first WLAN control stopped before C compilation for a missing normal
generated feature-grep header. The second helper attempt rejected an unused
generated `-I` search directory as though it were mandatory; that distinction
was fixed without source changes, with a dedicated tool test. The corrected
retry uses a private copy of the real generated forced-include header.

The entire copied WLAN tree was checked against baseline commit
`0c4d5b23ab7ca52462fd40cb1c9ecb02438a0fe0`: all6200 tracked C/header/Kconfig/
Bazel inputs match Git blob IDs. This includes host-cmn/fw-api/platform, not
just qcacld. The implementation/control remains frozen despite later live
source patches. Evidence: `wlan-peach-control/run01/full-wlan-baseline-source-proof.json`.

Actual WLAN compilation then stopped at a genuine callback boundary:
`wlan_hdd_cfg80211_set_mon_ch` is selected as a two-argument callback by its
Linux-version guard, but the current6.12.114 headers declare three arguments.
No cast, warning suppression or callback source change was made by these tools.

Stock ELF confirms the real production boundary: stock cfg80211 invokes
ops+0x138 with wiphy in x0 and chandef in x1, checking KCFI `0x29b128f3`;
stock peach's callback has that same KCFI identity and consumes incoming x1 as
chandef. The source transition is local `d6aef302c41802ca7eaa5a40e476a43beeb27265`,
upstream `9c4f830927750a2bf9fd9426a5257f0fdce3b662`, stable
`4dae80dd259b5c24f6c657f434afd5de1f791d12`. It is preparation for multiradio
monitor operation and a stable dependency, not permission to undo later fixes.

The current source three-argument branch is guarded by Linux>=6.13, while this
tree has the API backported into6.12.114. Simply selecting that branch would
not preserve the stock two-argument runtime contract. Stock cfg80211's normal
vendor_dlkm and dormant vendor_boot copies are identical SHA256
`6e9d3d678624eb2e22f93799b9196e3591fd134a880fe716209a7c4348c277a7`.
It has373 stock import edges across mac80211, kiwi, peach, wcn7750 and wonder.
The minimum callback unit is cfg80211+peach; a replacement provider requires
reviewing all five actual consumer interfaces/modes and any further necessary
closure. An independently proven source-matched stock-module SDK is a different
possible design, not implemented here. Do not repair this with a cast or CRC
forcing. Exact disassembly/provenance is under `wlan-provider-boundary/`.

The new source-matched WLAN candidate is frozen at
`09decca5f42a2742401cacbcd1a1d4a6c0fd67b3`. Its broader final provenance proof
checks6215 committed code/config/build inputs across the complete WLAN tree:
6213 match their committed bytes, and only the source Makefile/Kbuild are
replaced by the normal DDK-generated files, both byte-identical to preparation
outputs. No unexplained source difference remains. This larger count reflects
the proof's expanded build-definition scope, not15 unexplained source additions.
The actual new ELF callback is the three-argument
`wlan_hdd_cfg80211_set_mon_ch_with_netdev`, KCFI `0x7e862964`, and its code takes
chandef from x2. This confirms correct header-based selection but also confirms
it must not be deployed against stock cfg80211's two-argument/KCFI
`0x29b128f3` call boundary. All838 C compilations and the565,810,872-byte
aggregate ELF link completed successfully; final modpost correctly failed with
84 unresolved CNSS2/utils APIs because their legitimate producer SDK tables
were not yet built. No modpost error was suppressed. The candidate remains
quarantined, not a complete `.ko` build or stock-compatible WLAN PASS.

The module-scoped compiler dependency record contains2702 actual source/header
inputs and zero unresolved names. The unmodified CNSS2 SDK diagnostic also
compiled and linked its aggregate object, then stopped at modpost for the
firmware-service, IPC-QMI and CNSS-utils closure. Genuine SDK prerequisites are
now built successfully, including SMEM mailbox, firmware-service and IPC-QMI,
then CNSS-utils and CNSS2. They are SDK inputs only, not runtime replacements.
The exact frozen WLAN candidate now fully builds **PASS** in
`attempt-minimal-sdk02`, with all normal modpost/`.mod.o`/link steps. Its earlier
84-unresolved and then10-unresolved attempts remain preserved; the latter
needed the two omitted genuine cached QMI/SSR provider tables, not source
changes. The final normal Module.symvers is empty because both stock and new
peach modules export zero symbols. Final module SHA256 is
`cd959ed843711ab9b8a4ac7f431f7f920df7bd2db355f9b8861f6f303c9366b0`.
The completed compiler-input record covers2709 dependencies with zero unresolved
names, and the entire frozen source closure remained unchanged.

The older retry touched an
unchanged flags-file timestamp and normal Kbuild therefore recompiled objects;
the helper now preserves unchanged mtimes for future retries, but no cache
timestamp was manipulated to skip the current compile. No stock ELF-derived
CRC table or ignored-modpost workaround is permitted.

**This compiled module is not stock-compatible and remains quarantined.**
Its601 undefined ELF symbols/602 version records compare with599/600 in stock.
The final actual retained-stock-provider audit finds:

| Import | Stock provider CRC | Compiled consumer expected CRC | Result |
|---|---|---|---|
| cfg80211_new_sta | 0x83d71e16 | 0xe494dd96 | CRC mismatch |
| cfg80211_del_sta_sinfo | 0x83d71e16 | 0xe494dd96 | CRC mismatch |
| wiphy_new_nm | 0x7640d963 | 0xc67e1f4d | CRC mismatch |
| system_percpu_wq | CRC matches | 0x571c73d5 | Not permitted by compiled unsigned-vendor policy |

There are zero protected export collisions and zero retained consumers of
peach exports, because it exports none. New imports `__hrtimer_get_remaining`
and `ip_send_check` resolve correctly. The audit helper's four combined policy
findings include the three CRC-resolution failures above; there is one
additional genuine permission failure, not four independently forbidden APIs.
These actual boundary failures are in addition to the independently proven
old/new callback KCFI mismatch. No whitelist addition, CRC forcing or callback
cast was used. Nothing from this WLAN/CNSS SDK closure enters the four-provider
hybrid population. The original unmodified control could not compile against
the newer headers, so do not claim a completed patch-only binary control/ABI
comparison from it. Exact evidence is `stock-boundary-audit.json` and the
preserved original failure, not a runtime qualification result.

SDK modules are never automatically runtime replacements. The original17
cached SCM/IOMMU/ramdump and7 IPA USB-tether mismatches remain recorded for
their own groups. Real unedited tables may support another group only when
every actually consumed API agrees with retained stock. New imports and private
callbacks/layouts must be checked after every build.

## Fresh full-build and ABI gates

The exact command `tools/bazel build --jobs=8 //common:kernel_aarch64` completed
**PASS** under `common-production-01/` in1756.436s. Strict KMI and the symbol-list
violations marker were produced successfully by that exact command. Its source
HEAD is `e65991a0500c64fedacfc9a1e540c952d36c98e2`; common implementation/config
tree is the fixed `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22`. The later frozen
driver source HEAD is `09decca5f42a2742401cacbcd1a1d4a6c0fd67b3`, with that same
common tree. The log records eight BTF deduplication warnings about multiple
type IDs; these are disclosed warnings, not a successful STG comparison claim.
This is a fresh command, not a borrowed earlier PASS. The old qualified
artifacts are not replaced by these outputs.

The follow-up common ABI dump completed **PASS** at final frozen driver HEAD
`09decca5f42a2742401cacbcd1a1d4a6c0fd67b3`, elapsed1554.943s. Normal Bazel
processing legitimately started another KernelBuild for this invocation; no
forced SCM/environment override was used to manufacture a cache hit.
The fresh effective common `.config` hashes to
`e5495ab30241f869f12b3fc2ff3590886150fb464c736975c8054303a9e9981a`, exactly the
frozen qualified config: zero config-byte delta. Fresh completed common
Module.symvers contains10668 records and hashes to
`6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1`, exactly the
frozen qualified file; actual vmlinux-only symvers also matches byte-for-byte
(`fa6d325e5e5ce232ca7622176829ce7c4bcf7cf7ea47070bf6f48b265da02efb`). This is
a fresh output comparison, not an inferred result from source/config equality.

Both full and filtered fresh common STG dumps match the frozen qualified
five-CVE dumps byte-for-byte. The normal enforced comparison was still run and
returned **FAIL/exit4**: all five fresh reports are byte-identical to their
frozen counterparts, preserving exactly38 reviewed differences and zero new
differences. Do not relabel this comparison PASS. No ABI reference was updated.

The fresh validation-only Image SHA256 is
`8be0317b7e368a6a67af2ffa6463e9597325cd0c67c5967f4ecc8ec39a70e3d3`, release
`6.12.114-android16-6-o-g09decca5f42a-4k`. It is not the physically qualified
Image and was not packaged as a deployable ColorOS candidate or flashed. Normal
generic production validation does construct its own non-deployable boot/image
containers; those are not a normalized firmware-qualified release package.
Private immutable snapshots and actual hashes are in
`common-abi-dump-01/kernel-output-snapshot/`.

Normal `tools/bazel build --jobs=8 //soc-repo:canoe_perf_dist` completed **PASS**
at the frozen09decca5 source in1207.322s. This proves that exact build command
succeeded; it does not qualify the generic module/image population for stock
ColorOS deployment. Its eight depmod warnings affect six distinct modules,
exactly the same eight edges as the frozen qualified production log (zero new
or removed warning edges):

| Generic module | Missing dependency in generic population |
|---|---|
| charger-ulog-glink.ko | get_boot_mode |
| leds-qcom-flash.ko | qti_battery_charger_get_prop |
| msm_memshare.ko | md_buffersize |
| qcom-hv-haptics.ko | register_hboost_event_notifier |
| qcom-hv-haptics.ko | unregister_hboost_event_notifier |
| qcom_q6v5.ko | mdmreason_set |
| qcom_q6v5.ko | upload_mm_fb_kevent_limit |
| wcd_usbss_i2c.ko | upload_mm_fb_kevent_limit |

Generic images are not deployable replacements for the independently validated
exact stock/hybrid population. The current logs establish six modules rather
than inheriting an older approximate seven-module description.

The normal `//soc-repo:canoe_perf_abi` command completed **PASS**, elapsed
2545.936s. Its `with_vmlinux` transition legitimately built its own common-kernel
and module context; no flag override or borrowed PASS replaced it. This
context's common config,10668-record Module.symvers and vmlinux-only symvers
also exactly match the frozen qualified hashes. Both full and filtered Canoe
STG dumps are byte-identical to the frozen qualified dumps. Their hashes are
`530bdee82f59484a2de7bdb209e538167818524ea444cfcdff7a9d44e766d350` (full) and
`409b6a3b136298ae023acc4372d05f44a1f150705ca0adff9f433f26c37f126a` (filtered).
Canoe ABI is dump generation, not an enforced stock-module compatibility diff.

Finally, the exact normal common/strict target was rerun at source
`09decca5f42a2742401cacbcd1a1d4a6c0fd67b3`: **PASS** in2.590s,10 legitimate
action-cache hits and1 normal action. Its strict-KMI and symbol-list-violations
markers were both produced; Image/Symvers/config hashes remain those recorded
above. This final command, not the earlier e65991a SCM invocation, closes the
exact final-source common/strict gate.

The normal production target is not automatically a build of every vendor
techpack: `define_techpack_modules()` is currently an empty return, while the
registered module/ABI sets contain SoC modules and their explicit OPlus set.
No qcacld/peach entry occurs in these target lists. Do not use a generic Canoe
PASS as a substituted WLAN/media/SecureMSM targeted-control PASS. The first
unconfigured `somepath` query failed because it expanded an unused
`//msm-kernel` select alternative absent from this repository. That failure is
preserved, not treated as exclusion proof. The correct configured query then
succeeded with an empty result and zero build actions:

```sh
tools/bazel cquery --output=label \
  'filter("qcacld-3[.]0:canoe_perf_qca_cld_peach-v2", deps(//soc-repo:canoe_perf_dist))'
```

This confirms peach-v2 is absent from the actual configured production closure.
No broad target enabling or guessed dummy `msm-kernel` package was added.

Normal standalone DDK preparation/compilation is not a fresh production target
PASS. Some baseline source/config/provider closures and old stock private
interfaces can block the full production target even when one self-contained
component compiles. The initially omitted57 OPlus display files were recovered
byte-exact from their existing manifest and committed separately at
`4237c376`; no dummy header or feature disable was used. The restored display
controls and their subsequent genuine SDK closure are recorded in the
separate multimedia ledger; do not infer their runtime fit from generic build
success. No feature or validation was disabled to complete these gates.

| Final gate at source09decca5 | Result |
|---|---|
| Exact common build and strict KMI | PASS |
| Common ABI generation | PASS; both STGs byte-identical to qualified baseline |
| Enforced common GKI comparison | FAIL/exit4;38 reviewed existing differences,0 new |
| Production Canoe distribution | PASS;8 pre-existing generic dependency-warning edges |
| Canoe ABI generation | PASS; both STGs byte-identical to qualified baseline |
| Build-infrastructure host tests |20 PASS |
| WLAN matched-source compilation | PASS; quarantined for3 stock CRC failures,1 genuine permission failure and incompatible private callback |
| New device/driver runtime qualification | NOT TESTED |

## Capacity, packaging and physical scope

At build preflight the host had about322 GiB available,30 GiB RAM and22 GiB
available RAM; module makes were bounded by jobs and Bazel preparation was
serialized. The common command uses Bazel `--jobs=8` for outer action scheduling;
the normal inner kernel make was observed with `-j4`. The first WLAN candidate
used2 make jobs; subsequent genuine-SDK retries used1. Other component makes
were separately bounded.
Preserve successful/failed controls and frozen original outputs; no
automatic cleanup of user caches or old qualified artifacts was performed.

Root owns capacity discovery/validation. Exact stock file sizes are
154849280 bytes vendor_dlkm and14512128 bytes system_dlkm. Neither is evidence
of live logical partition capacity, super allocation/group free space, snapshot
state or slot mappings. No live extent/snapshot state is invented. Any offline
image feasibility must clearly distinguish source-filesystem geometry from
device-fit authorization.

The completed normal production target also generated an explicitly synthetic
AOSP sparse super fixture, not stock metadata:570425344 expanded bytes,
generic group566231040 bytes, generic system_dlkm98353152 bytes and generic
vendor_dlkm179417088 bytes. File size277209352 bytes,18 chunks and SHA256
`8a61a1c3511fa9bb16a353b42abd266b90b0a9447a4bbf45e10e79185bdba26f`.
It is useful for testing an LP parser against real `lpmake` output but must
never establish actual-device capacity, snapshots or installability.
Exact tool command/evidence is `build/generic-super-{fixture.json,commands.log}`.

AVB uncertainty does not prevent source review, module builds or explicitly
scoped unsigned offline filesystem feasibility. It still blocks a deployment
claim. Stock AVB policy and top-level vbmeta remain unchanged; these tools made
no device write. Generic generated containers are not deployment artifacts.
Final installable images and physical qualification require separate
partition/authority gates; nothing in this ledger qualifies a release.

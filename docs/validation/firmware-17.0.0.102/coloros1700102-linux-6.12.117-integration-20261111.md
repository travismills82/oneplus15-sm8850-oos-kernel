# ColorOS 17.0.0.102 Linux 6.12.117 integration — 2026-11-11

Status: offline-qualified experimental boot-only candidate; subsequent
authorized boot/read-back, Android/FBE and live feature checks PASS. See the
[physical canary record](coloros1700102-linux-6.12.117-physical-canary-20261111.md).
The owner subsequently reports radios, camera/video, calls, fingerprint and
USB sharing PASS, with a clean read-only reconnect scan at about 16 minutes.
Remaining hardware/recovery and extended-power qualification is pending.
Enforced GKI comparison:
FAIL, 53 reviewed difference owners.
This is not stable-release or GKI-compliance qualification.

Deployment: boot.img only; unchanged stock system_dlkm, vendor_dlkm,
vendor_boot, init_boot, DTBO and parent VBMeta. The offline no-write stage was
followed by owner-authorized testing on active boot_a only. Other devices and
emulators are not test targets.

## Preserved baseline

- Source/main: `8ea149a53695e05a6b7ebe9ed720a1b33a365813`.
- Tested kernel: `6.12.116-android16-6-o-gc2e2f95d3c3c-4k`.
- Boot SHA256: `f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16`.
- Image SHA256: `456436bea4d1d1f126b4ebfce9001253871f4b0edf1d2af07c8116f836428038`.
- Config SHA256: `da9bf33efeac03dd29e0cefb9fb2f6aaad272b8b2458886fb0e30c06840656c5`.
- Module.symvers SHA256: `e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8`.
- Common source tree: `481a38e99f82a476cdfa1ec1023cd9d4232dcaae`.

The complete 96 MiB rollback was copied and SHA256 verified. Main/origin/main
remain unchanged. The unrelated dirty historical root checkout was not reset,
cleaned or used as a build input.

## Frozen candidate

- Branch: `experiment/coloros17-linux117-boot-only`.
- Build source: `9218fe3f104bda23fb7e638f3f4a7236794b598b`.
- Common tree: `2e69273bcb66991b801cdce148ff66a7958e08e1`.
- Release: `6.12.117-android16-6-o-g9218fe3f104b-4k`.
- Scope: OnePlus 15, ColorOS `CPH2747_17.0.0.102(EX01)` only.
- Boot: `/home/travis/Android/stock_kernel/out/coloros17-linux117-build/candidate/boot.img`.
- Boot bytes: 100,663,296; qualified 96 MiB contract.
- Boot SHA256: `3cef6c673b60f189d19c95113a72e49c924a2f2800d8726b3b43f0fe1e8d4c51`.
- Image bytes: 43,309,568.
- Image SHA256: `2e9a03d847908563c07f56338b3e9a0a9a8af7486a63d99742479552789b9091`.
- Module.symvers SHA256: `af4d9aab9fb20ed169d6251f58af3579fa32fa1d3f3e6f346b7d4686224e7371`.
- Config SHA256: `902bfd6eff25d941cf2a19791022ea3a8cdd338fd6801b793b62c54b81a83ded`.
- Generated ABI SHA256: `935602a0cf5795fd82e26fce78e06298bf59c23a4c0b30f23b9d6abdd7a18971`.

The candidate directory contains only boot.img, README.md, manifest.json and
SHA256SUMS. Frozen kernel outputs and evidence are in local
`out/coloros17-linux117-build/` (E below). Documentation after the build does
not alter the payload or recorded build-source identity.

## Official stable provenance

Linux stable interval:
`5c40a1750f44dd6c7292d8c8d0b4ffff1734576e..d3417611a3b40b7f04a29e558cebac2bf69889d7`.
Tag object: `3edfb92cd34f439b2e2878c3544a1683871d29ee` (v6.12.117).
The official Git ref and [kernel.org changelog](https://cdn.kernel.org/pub/linux/kernel/v6.x/ChangeLog-6.12.117)
agree. This is Linux stable, not a claimed new ACK tag merge. GPG verification
could not establish trust because the signer's public key is not installed;
signature verification is NOT recorded as PASS.

Of 1,535 ordered commits, 1,525 were imported individually with original
authorship, author date, subject and original-SHA provenance. Ten equivalent
already-present fixes were inventoried, not duplicated. Fourteen downstream
context adaptations were reviewed. One coherent infrared prerequisite was
restored separately while preserving shipping rc_dev storage. No squash.

See the [full source-adaptation ledger](coloros1700102-linux-6.12.117-source-adaptations.md).
E/upstream-commit-verification.json and verify-upstream.py preserve the complete
machine-readable ledger. Imported stable head is
`ddfd2b640f6d0eed246bb00e3d1b544f2ce1e927`.

## Downstream compatibility commits

| Root | Source correction and safety | Local commit |
| --- | --- | --- |
| USB/driver core | Keep 48-byte device_type; typed parent-lock callback consumes actual bus reserve 1. Only USB interfaces lock the parent; no read past old objects or hub-parent UAF | c9a8e8a0680d8 |
| IRQ destructor | Real callback work fits old 128-byte irqaction padding; retain completion before free | dd498a85d3602 |
| remoteproc | Real subdevice-start state fits old padding; recovery semantics and 1,432-byte size retained | 4e1ec786696af |
| HID | Report offset consumes reserve 1, retaining 7,856-byte object and old offsets. New hiddev refcount fits old padding; UAF fix retained | ca3bdbd551550 |
| DBS | Maximum sample load fits tail padding at byte 180; flags and 184-byte object retained | f6590a5186036 |
| Rust bus | Name consumed union as old reserve member; retain Rust parent identity and storage; omit only unused generated Default methods, not definitions | aa31795c94465 |
| Rust aliases | Six scoped Android KABI type descriptions retain compatible legacy aliases while keeping safe .117 KCFI code generation | 9218fe3f104bd |

Infrared prerequisite `7f8773cb0578a` originates from stable/ACK
`75977f5f5c4974c72bbedefbf09ad33ebbaa36f1`, upstream
`dccc0c3ddf8f16071736f98a7d6dd46a2d43e037`. It separates unregister/quiesce
from final raw-state free, updates all affected in-tree callers, and retains
managed_alloc as legacy storage. A context-only BPF backport would not preserve
this lifetime/locking contract. Stock importers of affected allocation,
unregister and free interfaces: zero.

### Rust version and KCFI safety

Stable `dd2275f25607963f84ef98379fffc350ab282bae`, local
`64e60779c6de69dece43648d49fff17e4efc78fe`, adds
`-Zmerge-functions=disabled` under Rust KCFI. It prevents unsafe folding of
functions with different KCFI types and remains enabled.

With identical compilers this changes six old alias-based gendwarfksyms
descriptions imported by stock Rust Binder. Legacy descriptions were recovered
using reference-only old-policy object compilations, NEVER linked into the
candidate. Four formatting interfaces have unchanged full source type graphs,
ARM64 widths, call ABI and actual KCFI prefixes. Two Defaults have identical
instruction bodies but intentionally corrected KCFI prefixes:

| Interface | .116 prefix | Safe .117 prefix | Stock Binder use |
| --- | --- | --- | --- |
| rb_node Default | 0xfe088a00 | 0x934a9910 | 14 direct CALL26 relocations, no pointer relocations |
| rb_root Default | 0x54c485d6 | 0x4e43300a | 13 direct CALL26 relocations, no pointer relocations |

Six ordinary ANDROID_KABI_TYPE_STRING rules describe the genuine old compatible
external types, not numeric CRC values. C assertions prove ARM64 alias widths,
alignment and return storage. Kbuild adds ONLY the discarded ABI-rule section
to newly compiled objects before ordinary gendwarfksyms processing. Actual
normal-build before/after objects have identical allocated runtime/KCFI bytes,
symbol semantics and relocations. A rejected ld-r experiment is correctly
rejected by the same comparison as a negative control.

Evidence: E/normal-rust-metadata-proof.json, rust-metadata-negative-control.json,
six-rust-source-type-audit.json, six-rust-kcfi-prefixes.json,
default-functions-body-identity.json and six-rust-binder-relocations.json.
No numeric CRC forcing, Module.symvers editing, consumer version patch,
dummy function or security weakening occurred. Production gendwarfksyms is
unchanged.

## Verification

| Gate | Result |
| --- | --- |
| Stock binary identity | PASS: all 1,070 extracted .102 modules match inventory SHA256s |
| Actual C ELF/string checks | PASS: 1,070 modules, 7,490 negative mutations; ASAN/UBSAN host harness, not live module loading |
| Historical host suite | 201/202 PASS; old .115 whole-file futex freeze assertion FAIL |
| Focused .117 source/control-flow suite | 17 PASS, including exact upstream futex substitution and actual C USB predicate under ASAN/UBSAN |
| Build 01 | Deliberately cancelled for ABI review, exit 8; not PASS |
| Builds 02/03 | PASS; diagnostic ABI differences remained, not final candidates |
| Fresh-cache build 04 | PASS, 1,436.488 seconds; six Rust differences remained |
| Final build 05 | PASS, 204.049 seconds, source 9218fe3f104bd |
| Strict KMI symbol-list check | PASS |
| Common ABI generation | PASS |
| Compiled existing exports | PASS: all 10,676 records preserve CRC/provider/class/namespace; one additive __skb_vlan_pop export |
| Private layouts | PASS: 17 core and 13 additional driver roots, real reserve/padding use verified |
| Shared KCFI symbol records | 2,535 unchanged; actual Rust prefixes separately audited above |
| Full stock-module oracle | PASS: 1,032 compatible, 38 dormant, zero blockers |
| Production-provider policy | PASS: zero mismatches, missing providers or protected-export collisions |
| Enforced common GKI | FAIL: 45 unchanged prior owners plus eight new reviewed owners; zero unexplained new owners |
| ARM64 Droidspaces VM | PASS on final Image: namespace, seccomp, cgroups, IPC, ext4/loop, overlay and devtmpfs smoke tests |
| ARM64 TC VM | PASS on final Image: CAKE/FQ/FQ-CoDel each eight create/traffic/stats/delete cycles |
| Config/toolchain | PASS: all 6,573 effective options unchanged; pinned Clang/LLD/Rust/bindgen hashes match .116 |
| AVB packaging | PASS offline: qualified header/properties and exact frozen kernel bytes |
| OnePlus 15 physical testing | Subsequent boot/read-back, Android/FBE and live feature checks PASS; full hardware/power pending |
| Generic Canoe distribution / Canoe ABI | NOT RUN; no generic DLKM output used or claimed qualified |

The historical failure is an obsolete byte-identical .115 futex/fork file
assertion. The focused .117 check proves the intended upstream locking change
while retaining downstream identity handling. The old suite is not relabeled
PASS; no security fix is reverted to satisfy it. VM tests are not physical
device or full-distribution tests. Optional macvlan, IPv6 NAT and pids limits
remain absent as in the baseline.

Exact production command, from the linked worktree's kernel_platform:

```sh
tools/bazel \
  --output_user_root=/home/travis/Android/stock_kernel/out/droidspaces-build/bazel \
  build --jobs=4 --config=local \
  --cache_dir=/home/travis/Android/stock_kernel/out/coloros17-linux117-build/fresh-kbuild-cache \
  --kbuild_symtypes --show_progress_rate_limit=15 \
  //common:kernel_aarch64 //common:kernel_aarch64_abi_dump \
  //common:kernel_aarch64_abi_kmi_symbol_checks
```

The same flags with run //common:kernel_aarch64_abi_diff produced FAIL, exit 4.
The comparison parsed successfully; no missing-tool/malformed-reference issue.
No ABI-reference update was run.

### Exact module and loader result

```text
Modules:                     1,070
Active compatible:           1,032
Dormant:                        38
Hard imports:        60,701 / 60,701
Weak imports:                     6
Versioned imports:  60,707 / 60,707
CRC records:        61,777 / 61,777
Missing providers:                0
CRC mismatches:                   0
Namespace failures:               0
Active blockers:                  0
Production mismatches:            0
Protected export collisions:      0
```

The 3,220 provider ambiguities are separately checked against linked
vmlinux-first/protected-export policy and actual stock providers. All 54,785
vmlinux-first edges and 54,986 unsigned policy edges pass. Negative controls
pass. No rebuilt Bluetooth, PPP, GPU or other module substitutes stock.

### Enforced GKI review

All 45 prior owner reports are unchanged. Eight new owners are reviewed:

| Owner | Classification and actual module impact |
| --- | --- |
| bus_type | Typed callback in old 8-byte reserve at byte 168; size 200 and public offsets preserved |
| Rust bus_type | Same named reserve union/storage; no Rust user of new callback field |
| irqaction | Real destructor work in old padding; size 128 and previous members unchanged |
| rproc | One-byte recovery state in old padding; size 1,432 unchanged |
| hid_device | Report offset in reserve 1; size 7,856 unchanged |
| policy_dbs_info | Sample-load state in old tail padding; size 184 unchanged |
| io_sq_data | Private built-in SQPOLL lifetime evolution, 168 to 144 bytes; no stock helper imports/vendor source consumers; all code rebuilds |
| vfs_getattr_nosec | STG debug-type omission, not function removal. Exact prototype, ELF function, CRC 0x65883b5f and KCFI 0x9b621f5b retained; two stock oplus_overlay import records MATCH |

Raw reports: E/baseline-gki.report.small, candidate-gki.report.small and
enforced-gki-delta.json. Scope is a reviewed exact-stock boot-only experimental
canary, not GKI compliance PASS.

## Firmware, security and packaging invariants

Bluetooth/PPP/rfkill/slhc stay stock modules, not built-ins or replacements.
Stock GPU and GEAS remain providers. No module-shadow loader, signing-policy
or certificate change, KMI list change or ABI-reference update is included.
KSU/SUSFS, Droidspaces, CAKE/FQ/FQ-CoDel, watchdog fixes and all effective
options remain unchanged. Fixes only compiled into non-deployed modules do not
update stock binaries; this is not full firmware/DLKM security remediation.

Qualified AVB contract: Android v4 header, empty ramdisk/cmdline, no new
DT/signature payload, 100,663,296-byte partition, SHA256_RSA4096, flags/rollback
index/location zero, existing development boot key. Properties: OS version 16,
security patch 2026-10-01, fingerprint
qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys.
ColorOS naming is not permission to alter the Android 16 vendor/KeyMint
contract. Offline signing is not OEM authority or a physical AVB PASS.

avbtool verify_image passes on correctly named boot.img. Header bytes match
qualified .116 except the expected kernel-size word. Extracted kernel SHA256
matches frozen Image. One initial packaging verification used a renamed
rollback and could not locate descriptor sibling boot.img; it was rerun on
the correctly named original, with rollback-copy SHA separately verified.
No validation was bypassed.

## Pending controlled physical test

Freshly verify raw OnePlus 15 identity, firmware, ORANGE/unlocked state,
current slot/mappings, snapshot/update health, user0 RUNNING_UNLOCKED, F2FS
inlinecrypt, stock companion partitions and unchanged KernelSU init_boot.
Back up and hash-verify the complete active boot; verify recovery/fastboot
rollback access.

Write ONLY verified active boot using the frozen candidate. Read back all
100,663,296 bytes and require its exact SHA256. Never write DLKM, VBMeta,
init_boot, vendor_boot, DTBO, slot metadata, userdata or encryption metadata.
On mandatory boot/FBE/module failure, preserve logs and restore qualified .116.

Observe startup, PIN/user0, KeyMint, F2FS inlinecrypt, module loading,
KSU/SUSFS, container/TC features and kernel diagnostics through at least 240
seconds. Wi-Fi/Bluetooth, LTE, camera/video, audio/microphone, fingerprint and
OTG are NOT TESTED until actually performed. USB-disconnected idle/suspend
needs separate observation, not a USB-connected idle proxy.

No push, merge, release or main-branch change is part of this task.

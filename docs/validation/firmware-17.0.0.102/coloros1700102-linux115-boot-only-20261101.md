# Repaired Linux .115 / stock ColorOS .102 boot-only canary

Date: 2026-11-01. This record summarizes the completed restoration and the exact
artifact subsequently selected for canary publication. Publication performs no
new device writes, kernel builds or image repackaging.

## Artifact and source identity

| Item | Value |
|---|---|
| Firmware / tested device | `CPH2745_17.0.0.102(EX01)` / CPH2745IN, OP611FL1 |
| Active slot / trust state during test | B / unlocked, ORANGE |
| Kernel release | `6.12.115-android16-6-o-ga3b501fee3c3-4k` |
| Kernel build source | `a3b501fee3c3c9e44579f46fd78e2282d05e29e8` |
| Common source tree | `c74ab6f64f685145302559ce9fa2a2b00dc4c385` |
| Boot bytes | 100,663,296 |
| Boot SHA256 | `7eba8fe7265c316265a274c893ae7886ba4598942862fb642d0b34fc61bd8e6e` |
| Image SHA256 | `417dd7d89e159ecf6d0f8e7032ac6f44b79d2589c3c1b6ccd92dc47b170cf30a` |
| Module.symvers SHA256 | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Final config SHA256 | `abbb37e8fae9073bd5ca5bf539196f4b9baff60fa40714fe1506dc01592f1a27` |
| Prepared SDK manifest SHA256 | `98c38045969a0c06a2ce277c4a71ab3f6bab5f7b307702bc24359cbce8ea4953` |

The exact repaired Image was reused with the qualified .114 boot-only packaging
contract: Android v4 header, empty header cmdline, no ramdisk, qualified QTI AVB
properties, same development boot key, flags/index/location zero. AVB signature
and embedded Image checks passed. Stock-parent/custom-boot acceptance was
physically tested in unlocked mode, not asserted as locked OEM trust.

## Original stock module model

All module partitions use exact original ColorOS .102 binaries. No replacement
module, GEAS blacklist, module-shadow patch or modular-to-built-in conversion is
deployed. BT, HIDP, PPP, RFKILL and SLHC remain `m`; MODVERSIONS, MODULE_SIG,
GENDWARFKSYMS and CFI_CLANG remain enabled. NTFS3 is disabled.

| Offline gate | Result |
|---|---|
| Exact stock module locations hash-checked | 1,070 |
| Active compatible / dormant | 1,032 / 38 |
| Hard imports | 60,701 / 60,701 |
| CRC records | 61,777 / 61,777 |
| Missing / CRC / namespace / active blockers | 0 / 0 / 0 / 0 |
| vmlinux-first provider edges / mismatches | 54,785 / 0 |
| Compiled unsigned-import policy edges / failures | 54,986 / 0 |
| Protected-export collisions | 0 |
| Stock GEAS `get_gpu_bw_data` | `0xd8bbf8a1`, MATCH to stock KGSL |
| Preserved common build / strict KMI / ABI generation | PASS |
| Enforced GKI | FAIL: 38 prior reviewed + six representation differences |
| New full Canoe distribution rebuild for publication | NOT RUN; tested bytes reused |

The .115 core retains 231 individual stable imports, 15 already-present fixes,
the core KABI repairs and task-flag separation. All 10,163 vmlinux export CRCs
match the qualified .114 contract. The conntrack CVE-2026-80668 backport remains
present; no new complete security audit is claimed.

The pre-cleanup host repeat passed 215 tests in 22.802 seconds. Its first run
had one sanitizer-mutant test timeout; the unchanged focused eight tests passed,
then the full repeat passed with external DEBUGINFOD lookup disabled. Sanitizer,
assertion and compatibility checks were not disabled. Those 215 tests belonged
to the archived pre-cleanup packaging tree, not a new 215-test claim for the
cleaned release tree; the release scope fixtures are run separately.

Fresh cleaned-tree validation succeeded: `env DEBUGINFOD_URLS= python3 -B -m
unittest discover -s tools/driver-updates -p 'test_*.py'` passed all 202 tests
in 22.687 seconds, including the 20 boot-only scope fixtures. Scope validation
against the exact boot and frozen final config passed. The original qualified
boot signing key verified the frozen image's AVB signature and payload hash.

## Physical evidence

The full independent boot readback matches the published SHA. Original stock
parent vbmeta and both stock DLKM hashes were verified. The DLKM partitions
were already original and were not written during restoration; only active
boot and original parent vbmeta required writes. No release-publication device
action is performed.

Android boot, user0 RUNNING_UNLOCKED, CE/DE readability and F2FS inlinecrypt
passed. Vendor verity reported V0 with original OEM-authenticated root
`1374ff0726349e3ca91b2999a786c12af12530aff39a836c58dae68839f1669c`;
system verity was healthy. Nine critical loaded stock module identities matched,
including GPU, video, RMNET, crypto, audio and all three GEAS modules. Basic
Wi-Fi ICMP traffic passed 3/3. Health observation crossed 240 seconds.

Magisk init_boot, recovery images, inactive boot/parent, vendor_boot, DTBO and
super metadata were unchanged. No userdata/metadata format, encryption change,
AVB-disable flag, rollback-index change, key enrollment or slot switch occurred.

No panic, Oops, BUG, KCFI failure, RCU stall, soft lockup, unknown symbol or
module-version failure was found in the recorded fatal scans. Six early WARN
splats remain: known proc duplicate registrations, sysfs permission mode, HFI
IRQ wake accounting, HBP trailing-array Fortify and schedinfo constructor plus
`__GFP_ZERO` families. This is not an exhaustive new WARN or security audit.

Full hardware regression, TWRP PIN decryption, repeated boot and unplugged
suspend tests are NOT PERFORMED for this exact stock-DLKM boot-only artifact.
Hardware passes from the earlier updated-GPU/no-GEAS stack are not transferred.

## History cleanup and release decision

The owner explicitly approved resetting main from
`a320420562e2a55bb5db2cdf494579ca6898ee7f` to the exact build source above,
replacing the ten later packaging/documentation commits with one boot-only
release commit. The old tip is preserved locally as
`archive/main-before-boot-only-cleanup-20261101` and the existing feature branch.
No stable import, KABI repair, implementation/config change or ABI-reference
update is part of that cleanup. Existing tags are not rewritten or deleted.

Decision: **CANARY/PRERELEASE ONLY**. The exact boot-only artifact has positive
encrypted-boot and stock-module evidence, but extended hardware qualification,
known WARNs, enforced GKI FAIL and security completeness remain disclosed.
The .114 stable release remains latest stable.

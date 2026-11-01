# KernelSU/SUSFS uname-enabled physical canary — 2026-11-01

Scope: OnePlus 15 CPH2745IN / OP611FL1, ColorOS
`CPH2745_17.0.0.102(EX01)`, active slot B, unlocked/ORANGE. No universal model,
firmware, security-completeness or stable-release qualification is claimed.

Exact boot SHA256:
`3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`,
100,663,296 bytes. Frozen common source tree:
`be76448748b0d005af30e20235c5447b1e878dd0`, content-equivalent source commit
`bb4992ba6b0a7814b4eec02d4201a175478d6e42`.
Runtime release remains `6.12.115-android16-6-o-gf116d2166542-4k`; source was
committed after the physical test without a payload rebuild.

## Physical results

| Gate | Observed result |
|---|---|
| Previous working active boot backup | Complete, SHA256 `889072fec1140bdd56f9ec6390d914f310ef8bdf949f6353accbdf2b69326470` |
| Write scope | Only boot_b; no slot/trust-state change |
| Complete independent host readback | Exact new boot SHA256 match |
| Protected partition hashes | Inactive boot, both init_boot/vbmeta/recovery slots, vendor_boot, DTBO and both DLKM unchanged |
| Recovery installed-kernel config | SHA256 `8a1b910d9ad4ba7490c7bf06b01d7d2ddf79deda064bda4c277553a025fb0d5f` |
| Root read-only getters in recovery | KernelSU32716/UAPI6, SUSFS v2.3.0/GKI; uname support explicitly advertised |
| Android startup | sys.boot_completed=1 |
| User0 | RUNNING_UNLOCKED |
| /data | F2FS, inlinecrypt mounted |
| SELinux | Enforcing |
| Android running config | Exact same new config hash; KSU/SUSFS/SPOOF_UNAME=y |
| Stock module initialization | Bluetooth/HIDP/PPP/RFKILL/SLHC/KGSL present; stock GEAS retained |
| Basic connectivity state | Wi-Fi address, Bluetooth enabled; not a complete hands-on test |
| Peripheral/suspend/gaming/root lifecycle | NOT TESTED for this artifact |
| Arbitrary uname spoof values | NOT CONFIGURED / NOT TESTED |

Recovery logged the pre-existing stock hbp frame_put warning, also observed
with the preceding uname-disabled build. It is not suppressed or represented
as resolved. Recovery mounted encrypted data; after its command interface
reported busy, normal ADB reboot returned successfully to Android. No kernel
crash or FBE failure was observed. Unprivileged Android kernel-log access was
unavailable; do not infer a complete clean-kernel-log qualification.

The newer official Manager APK was verified offline against the installed app
certificate and exact upstream pin, including the compiled API6 return value.
It was not installed as part of the kernel physical test. No root permission,
SUSFS hide rule, uname override, encryption policy or other partition was changed.

## Static qualification and installer boundary

Common build, strict KMI, ABI generation, 1,070-module stock oracle and linked
provider-policy checks passed. All 10,163 vmlinux exports and the filtered ABI
remain identical; stock counts are 1,032 active/38 dormant, 60,701 hard imports,
61,777 CRC records and zero module/provider blockers. Twelve compiled BTF
layouts are unchanged. Final host runs: 202 existing + 12 focused checks PASS.
One initial sanitizer-mutant test timed out; unchanged isolated and full reruns
passed with intentional host core dumps disabled. Failure evidence is retained.

Enforced GKI FAIL/exit4 remains unchanged: 38 reviewed prior deviations plus six
.115 representation differences. Full ABI-dist wrapper remains blocked by
missing libcap/libcap-ng selftest inputs; the normal enforced comparator ran.
No checker, CRC, signature, reference or KMI list was weakened/rewritten.

The release ZIP contains the identical tested payload and passes shell syntax,
archive/hash checks and 19 regular-file host scenarios including dry-run,
firmware/snapshot/stock-contract rejection, durable backup and rollback after
write/readback failure. The exact ZIP has **not been physically installed**.

Recommendation: firmware-specific **canary prerelease**, boot-only with original
stock DLKM/parent metadata and disclosed limitations. Not a stable promotion.
Retain the known-working boot backup and require additional hands-on hardware,
root-management and USB-disconnected suspend qualification before stable status.

Private raw logs, readbacks and rollback artifacts remain on the host under
`/home/travis/Android/kernel-test-backups/ksu-susfs-uname-canary-20261101/`;
device identifiers, PINs and personal logs are deliberately not published.

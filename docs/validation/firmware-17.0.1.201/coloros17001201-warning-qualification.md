# ColorOS .201 / Linux 6.12.118 — boot-warning qualification

Final disposition, frozen evidence, and the physical stability matrix:
[conditional qualification](coloros17001201-warning-conditional-qualification.md).

This continues the [boot-warning follow-up](coloros17001201-boot-warning-followup.md).
The new finding is a **required netd startup map creation**, not a failure
to reuse pinned maps. Both warnings remain deployment-blocked on .201.
Evidence and test sources are in `out/coloros17001201-warning-qualification/`.

## Installed baseline, unchanged

Read-only wireless ADB verified CPH2747 ColorOS 17.0.1.201(EX01), slot A,
`6.12.118-android16-6-o-g76b73bd515be-4k`, SELinux enforcing, and
`boot_a` SHA256
`8aecec4a35214a1ecb27e2170c28dcdd444542ac764bb3d84d3ab97447bfe16b`.
Stock rollback SHA256 remains
`e38045b1bdb0e51d71833206c848715cbc862f6fdc2039887cd7cd8248da5a4f`.
Both archived checksum manifests verified.

The corrected STG comparator again returned **exit 0** for the independent
ColorOS contract, with byte-identical filtered graphs. The unchanged-stock
oracle remains **1,031 active compatible, 38 dormant**, zero unresolved
CRC/provider/namespace failures. The existing enforced Kleaf GKI result
is **FAIL / exit 4, 76 report blocks**; it is a different check. No ABI
reference, required stock `arm_smmu` CRC contract, KernelSU/SUSFS,
Droidspaces, or Qualcomm driver configuration was changed.

## Persistent BPF/BHB policy

### Exact .201 evidence

The live decompressed `com.android.tethering` APEX, version `371021120`,
matches the signed OTA APEX byte for byte (SHA256
`4152937491c76b41b14fa57f112ab8470d636a4216aef4212d79fba53d8dce0b`).
Its `netbpfload` is the privileged init `bpfloader` service and
writes zero. The loader has `CAP_SYS_ADMIN` and `CAP_NET_ADMIN`;
it does not need unprivileged BPF to do its own loads. Rejection of that
write is fatal to the current loader path. The phone still reports 0 and
`Vulnerable: Unprivileged eBPF enabled` with BHB mitigation configured;
this is a reported residual exposure, not a demonstrated attack or
a conclusion that arbitrary Android apps can load BPF under SELinux.

**The exact loaded netd library is the second blocker.**
`/apex/com.android.tethering/lib64/libnetd_updatable.so` (SHA256
`b201bb4871e76bf68f096892e1358efb51dac1008f42d3b3d4d7637a284817f9`)
matches the signed OTA. Its init path calls ARM64 syscall 280 with
command 0 (`BPF_MAP_CREATE`), checks failure, and on success calls
command 2 (`BPF_MAP_UPDATE_ELEM`). The failure diagnostic is
`map create failed`. This matches the required array-map create/write
check in [AOSP BpfHandler::init](https://android.googlesource.com/platform/packages/modules/Connectivity/+/5f020a5ab853acf2a5a3ece263de716987c3f0d0/bpf/netd/BpfHandler.cpp).
No syscall was induced or traced by attaching to a phone service.

| Actor | Live UID:GID / SELinux domain | Effective caps, seccomp | What the evidence establishes |
|---|---|---|---|
| `bpfloader` / `netbpfload` | 0:root, `bpfloader`, init config (one-shot was stopped) | Init declares CHOWN, SYS_ADMIN, NET_ADMIN, SYSLOG | Privileged loading and pinning; the global write is not a loader capability requirement. |
| `netd` | 0:0 / `netd` | `0x74ef`, seccomp 0; neither BPF nor SYS_ADMIN | 35 BPF map FD references; OTA permits self map creation. Exact loaded library requires new map creation and update on startup. **Genuine dependency with stock credentials.** |
| `system_server` | 1000:1000 / `system_server` | `0x1a06887c20`, seccomp 2; neither BPF nor SYS_ADMIN | 52 map and eight program FD references, including framework/vendor consumers. OTA allows self map creation; pinned use is established, a separate required create was not proven. |
| Network stack | 1073:1073 / `network_stack` | `0x0800003c00`, seccomp 2; NET_ADMIN but neither BPF nor SYS_ADMIN | 11 map FD references. OTA allows self map creation; pinned-map operation is not evidence of creation. |
| Suspend service | 1000:1000 / `system_suspend` | `0x1000000000`, seccomp 0; BLOCK_SUSPEND | One map FD; exact binary imports object-get and lookup for the wakelock-duration map. OTA allows self map creation; this observed path does not require creation. |
| OPlus scheduler and CVT helpers | 1000:0 / `bpfloader` | `0x201001` each, seccomp 0; SYS_ADMIN, NET_ADMIN, CHOWN | Scheduler loads/attaches a libbpf skeleton, holds five map and four program FDs. CVT references the OPlus pin directory, no BPF FDs observed. These are privileged actors, not additional proof of unprivileged creation. |

The FD counts are references at capture time, not unique allocations.
SELinux facts above come from the exact OTA platform CIL and live domains.
The host analyzer could not decode the vendor live policy capability
encoding; this is not a complete live SELinux decision or dynamic call
audit. The targeted binaries and FD/credential receipts are retained
locally; absence of an observed operation is not proof no vendor path uses it.

### Isolated qualification

No policy write was made on the phone. The separate Android SDK emulator
was API 36 userdebug, x86_64 Linux 6.6.66, SELinux enforcing. The emulator
used its own AOSP services, not ColorOS firmware or Qualcomm hardware.
The app probe was signed solely with a local emulator test key. It is
not a replacement APEX.

| Check | Result |
|---|---|
| Policy 0 positive control | Wi-Fi and emulated cellular HTTP transferred the loopback test payload, app UID counters increased, VPN TUN came up, and Settings hotspot entered `TetheredState` with upstream. |
| Policy 2, netd restarted | **FAIL:** repeated `libnetd_updatable_init: EPERM: map create failed`, then netd `SIGABRT`. Init restarts zygote on netd restart, so framework availability failed too. Restoring policy 0 recovered netd and the app probe. |
| Policy 2, post-start only (same netd PID) | **PASS within scope:** Wi-Fi and emulated cellular each transferred the payload on a bound network; app UID RX/TX counters increased. VPN TUN received the deliberately routed local UDP test packet. Settings hotspot `TetheredState`, lastError 0, upstream selected and BPF offload map shim OK. |
| Tether client forwarding, hardware offload, VPN peer and physical modem | **NOT TESTED.** No hotspot client connected. The VPN packet was local to the TUN, not forwarded to a real peer. |
| Exact .118 ARM64 kernel capability boundary | **PASS:** guest root with the phone's netd mask gets EPERM on the ordinary array-map creation. Adding only CAP_BPF to that test subject makes create/update pass. The separate no-capability pinned-map test still passes get/lookup/update and denies new creation. No capability grant was made on the phone. |

This rules out a loader-only fix or a policy reboot recommendation based
on post-start traffic alone. A candidate must keep 2 while adapting
both the initial loader and the required startup creation check. Prefer
provisioning through a trusted privileged component with a real
least-privilege access check in the consumer. If netd must continue
creating its own maps, the ARM64 result supports evaluating **netd-only**
CAP_BPF plus a confined SELinux rule, retaining the rule against
unneeded program load/attach and not granting capability to app domains.
The kernel capability result does not verify that Android init, SELinux,
other service clients or vendor code will accept that firmware change.
It is not safe to just skip the test, ignore EPERM, or set irreversible 1.

### Signing decision

Verified the stock APEX container signature with `apksigner`
(Google Android certificate SHA256
`e53f52f414150c05bae0e4cee2073dd00fe644661d5f9a0fbe494adc07f05993`).
The payload hashtree and SHA256_RSA4096 AVB footer verify with the
embedded key, matching the packaged APEX key SHA1
`8f14e12d839a67f8f7ef36e66687e62ee6612785`. These are **two** distinct
signature checks. The preinstalled system copy is additionally
covered by the .201 system hashtree in OEM-chained `vbmeta_system`
(slot location 2). A properly authorized compatible Mainline/OEM APEX
update could preserve an unchanged system partition; replacing the
preinstalled system copy would also need an authenticated system
partition update. An unlocked bootloader does not supply the APEX
signing authorities. No compatible signed update or trusted signing
path was available. [Android APEX format and signing](https://source.android.com/docs/core/ota/apex).
No modified APEX, firmware image or verified-boot change was built or installed.

## HBP stock compatibility and source gate

Searched current [OnePlus SM8850 public refs](https://github.com/OnePlusOSS/android_kernel_modules_and_devicetree_oneplus_sm8850)
and the [.201 source request](https://github.com/OnePlusOSS/kernel_manifest/issues/231).
The OnePlus 15 public module tip is still
`4c03af1d6d462b9573a7a9f85b983c6bae49497a`. No exact .201 core +
Goodix/Synaptics source/SDK or OEM fix was found. The source request is
corroboration, not proof that private source does not exist.

`hbp-stock-contract.json` records all 12 exact stock exports, their CRC
and compiled entry KCFI (11 functions, one data export), namespace,
all observed importers, and both copies of the HBP core and four chip
modules (Goodix, Synaptics, OPlus T-series, Focal). Core depends on
`oplus_bsp_dft_olc`, `panel_event_notifier` and `oplus_bsp_tp_notify`.
All 12 HBP core exports have no namespace. Three exports are missing
from public core, and one extra public declaration
(`tp_gesture_enable_notifier`) is not in stock.

| Missing stock export | CRC | KCFI entry ID | Stock consumer |
|---|---|---|---|
| `hbp_get_device_id` | `0xf01ef47b` | `0xc069e464` | Goodix GT99x6, normal + recovery |
| `hbp_dev_set_irq_status` | `0x7de15933` | `0xdfeae308` | Synaptics S3910, normal + recovery |
| `hbp_exception_cancel_deferred` | `0xd272d446` | `0xe5c47d60` | No direct active consumer found; still an OEM interface |

The four actual chip ELF callback tables are 80 bytes with `get_frame`
at offset 40 and KCFI `0xed529916`, established from relocations and
entry prefixes, matching the shape of the public slot. This does not
prove every private member/callback layout: stock modules lack BTF/DWARF,
and the OEM core/other private layout and lifecycle contracts remain
unavailable. A single matching callback does not repair the two missing
active imports or validate a rebuilt dependency set.

**Bounds and warning:** the stock successful configuration accepts payload
size 1–8,192, total request 40–8,231. Its scratch request is payload size,
and the destination has total size with payload starting at 36.
The stored `total - 39` copy bound protects every coherent accepted
write length (verified across all 8,192 capacities); the read path
checks its requested length against total. For the 2,048-byte trace,
the coherent allocation requests are 2,048 scratch and 2,087 destination,
and the write ends at 2,084 exclusive. Stock uses `vzalloc`, not the
public `kzalloc`/4-KiB payload contract. This is requested-size evidence,
not a physical allocation trace or proof against the resize/IRQ lifetime
problem, unknown chip-producer behavior, or failure paths.

The field-spanning diagnostic is in the exact stock binary. Retained
warning logs identify the custom .118 kernel; no corresponding OEM
stock-kernel boot trace was found and live pstore was empty. Thus
**stock module defect verified; stock-kernel recurrence unverified**.

The flexible-array, error propagation and IRQ-serialization reference
patch was preserved (SHA256
`de4842f8283840acb87f0e435571e675d92a4ff5ca701ec27978a7f0f11d22bd`).
The prior real .118-header compile, KCFI/layout and internal-CRC checks
remain valid; the host ASan/UBSan wire and 201 allocation-failure checks
passed again. Full module build/deployment is **blocked** by the exact
.201 core/chip source, private layout/lock contract and matched vendor
SDK. No stock binary diagnostics were altered, FORTIFY remains enabled,
and no mismatched module or DLKM/AVB package was assembled.

**Disposition:** no warning-qualified replacement. The reversible
kernel BPF default and HBP reference repair are available as offline
work. Persistent BHB mitigation needs a trusted matched userspace update
and startup/dependency validation; the stock HBP repair needs matching
vendor source or an OEM-fixed module set. No phone reboot, flash, APEX or
DLKM replacement, push or publication was performed.

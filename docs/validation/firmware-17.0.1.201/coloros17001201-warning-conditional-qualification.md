# ColorOS 17.0.1.201 / Linux 6.12.118 — conditional qualification

**Disposition: retain the working boot-only baseline, with disclosed
warnings and incomplete physical coverage.** This closes the warning
investigation at the current evidence boundary. It does not qualify either
fix for deployment, establish comprehensive stability, or authorize a release.
See the [initial follow-up](coloros17001201-boot-warning-followup.md) and
[detailed evidence report](coloros17001201-warning-qualification.md).

## Frozen baseline and compatibility gates

OnePlus 15 CPH2747, `CPH2747_17.0.1.201(EX01)`, slot A. Source
`76b73bd515be1ed698c7df08ae29666eb3848b08`, kernel
`6.12.118-android16-6-o-g76b73bd515be-4k`.

| Artifact | SHA256 |
|---|---|
| Tested boot container / fresh read-only `boot_a` read-back | `8aecec4a35214a1ecb27e2170c28dcdd444542ac764bb3d84d3ab97447bfe16b` |
| Archived stock rollback / original `boot_a` read-back | `e38045b1bdb0e51d71833206c848715cbc862f6fdc2039887cd7cd8248da5a4f` |
| Raw kernel Image | `e9ea1f7a27e1d6dd073550f4fb3bfe8eee35b4748adab55a711b83889648cc9a` |
| Corrected independent ColorOS reference, filtered STG | `5522a748c5ae9f3597d0076bd3ab585be25d6697e6f79dcc30245d0139553736` |

| Gate | Result on the unchanged baseline |
|---|---|
| Strict KMI symbol list / common build / ABI generation | **PASS**, preserved build receipt. No rebuild was needed for this documentation. |
| Independent ColorOS ABI | **PASS / exit 0**; the previously compared filtered graphs are byte-identical with the corrected extractor. |
| Stock module oracle | **PASS / exit 0** on the actual installed source release: 1,031 active compatible + 38 dormant; 60,835 hard imports; 61,910 CRC matches; no missing providers, CRC mismatches, namespace failures or blockers. |
| KCFI and private layouts | Preserved targeted PASS: 33 linked layouts, 2,534 shared C type IDs and seven linked prefixes. This is not exhaustive coverage of opaque vendor structures. |
| Enforced GKI | **FAIL / exit 4**; 76 documented report blocks, not 76 independent roots. Keep the checked-in reference unchanged. Stock `vendor_boot/arm_smmu.ko` in normal and recovery requires `devm_tegra_memory_controller_get=0x67b44ae8` and `tegra_mc_probe_device=0x016da55a`. Google's comparison does not replace the ColorOS gate. |

Exact receipts are under `out/coloros17001201-linux118-preflight/` and
`out/coloros17001201-612118-gki-warning-evidence/`. The artifact and evidence
manifest is
`out/coloros17001201-warning-qualification/conditional-baseline.sha256`;
verify from the repository root with
`sha256sum -c out/coloros17001201-warning-qualification/conditional-baseline.sha256`.
The two boot files are in the preflight's
`deployment-readonly-20261121/{offline-experimental,active-a}/boot.img`,
respectively. No installed image, APEX, DLKM, verification setting or
rollback copy was altered.

## BPF/BHB: offline service-policy design

The installed Android 17 tethering APEX (version `371021120`) matches the
signed .201 OTA. At the physical checkpoint, `bpf.progs_loaded=1`,
netd was running, the knob was `0` and Spectre v2 status included
`Vulnerable: Unprivileged eBPF enabled`. BHB mitigation is configured;
this is a residual exposure diagnostic, not proof of an app-accessible
exploit. The frozen .118 config has
`# CONFIG_BPF_UNPRIV_DEFAULT_OFF is not set`: removing the userspace zero
write by itself does **not** establish an early `2`.

**Observed, not inferred:** the APEX's privileged `netbpfload` writes
`0` and treats an error as fatal. Its exact `libnetd_updatable.so`
requires netd to create and update a new array map during startup.
The stock netd mask `0x74ef` has neither `CAP_BPF` nor `CAP_SYS_ADMIN`;
policy `2` makes that kernel operation require BPF privilege. In the
isolated Android 16 emulator, policy `2` with **unmodified netd restart**
failed with `EPERM: map create failed`, then SIGABRT; restoring `0`
recovered it. The separate ARM64 capability probe using the .118-derived
offline Image confirmed that the stock mask is denied and adding only
`CAP_BPF` allows create/update. Pinned-object access was tested
separately without new-map creation. In the emulator, a switch to `2`
with netd **already running** allowed the tested Wi-Fi and emulated
cellular transfers, UID accounting, a local VPN TUN packet and hotspot
startup. Hotspot client forwarding, hardware offload and real VPN peer
traffic were not tested. None of these policy changes was made on the phone.

### Exact current service contract and proposed delta

The files below were read from the phone. Entries describe existing
configuration, followed by a **design only**, not an installable overlay.
See [Android init service options](https://android.googlesource.com/platform/system/core/+/refs/heads/main/init/README.md)
and the matching [AOSP map self-test](https://android.googlesource.com/platform/packages/modules/Connectivity/+/5f020a5ab853acf2a5a3ece263de716987c3f0d0/bpf/netd/BpfHandler.cpp).

| Stock service / owner | Exact relevant settings to retain |
|---|---|
| APEX `/apex/com.android.tethering/etc/netbpfload.35rc`: `bpfloader /apex/com.android.tethering/bin/netbpfload` | `user root`; groups `root graphics network_stack net_admin net_bw_acct net_bw_stats net_raw system`; capabilities `CHOWN SYS_ADMIN NET_ADMIN SYSLOG`; memlock soft/hard 1,073,741,824; disabled, oneshot, override; `reboot_on_failure reboot,bpfloader-failed`. Do not add capabilities or change the SELinux domain. |
| Platform `/system/etc/init/netd.rc`: `netd1shot /system/bin/netd oneshot` | `class main`; disabled, updatable, oneshot; `user root`, groups `root net_admin`; memlock soft/hard 1,073,741,824; `reboot_on_failure reboot,netd1shot-fail`. Do not add a capability to this entry without evidence that its separate mode creates a map. |
| Same platform file: `netd /system/bin/netd` | `class main`; disabled, updatable; `user root`, groups `root net_admin`; same memlock; socket names/permissions remain `dnsproxyd`, `oplus_dnsproxyd`, `fwmarkd` (stream 0660 root:inet) and `mdns` (stream 0660 root:system); existing zygote and zygote_secondary restart callbacks unchanged. |

Both netd entries currently declare
`CHOWN DAC_OVERRIDE DAC_READ_SEARCH FOWNER IPC_LOCK KILL NET_ADMIN NET_BIND_SERVICE NET_RAW SETUID SETGID`.
The narrow first prototype would append `BPF` to the existing capability
line of **only the long-running netd** service, giving expected effective
mask `0x80000074ef` if the other bits remain unchanged. No file capabilities,
app grants, `SYS_ADMIN`, or `PERFMON` for netd. A kernel capability grant
is broader than one map command; keep the existing `netd` domain and its
self-map creation and existing map-access controls. If a matched policy
build requires it, review only the initial-namespace grant equivalent
to `allow netd self:capability2 bpf;`, preserving the app capability
neverallow, the restriction of BPF program load to the loader domain,
and all pin access labels. Do not weaken a neverallow to make this build.
Trace actual program load/attach and FD operations in enforcing mode;
the fact that the ARM64 capability probe passes does not establish a
complete SELinux or Android-startup result. If further capabilities or
broader domain changes are required, stop and prefer a matched source
design that provisions the needed object in the existing privileged
loader without granting netd new privilege. Such a design must check
the actual required operations and enforce map ownership, access and
lifecycle; a dummy object or skipping the startup check is not acceptable.

1. In a **matched signed platform build**, make init's early action
   explicitly set `/proc/sys/kernel/unprivileged_bpf_disabled` to `2`
   before any relevant service starts or untrusted BPF caller can run,
   and before the existing `on load-bpf-programs` action. Confirm the
   live `proc_bpf` label and that init's narrowly scoped write is allowed
   by the exact policy; no new writable path for apps. Prove early event
   ordering on that firmware. This is reversible policy `2`, never `1`.
2. In the **matching APEX source**, replace the request for `0` with
   verification of `2` for this profile. Keep genuine failure handling
   and don't publish readiness if the policy or BPF loading fails.
   Keep the sequence: bpffs/APEX/logd ready → existing temporary kernel
   symbol-access setup → synchronous `exec_start bpfloader` →
   `wait_for_prop bpf.progs_loaded 1` → existing restoration →
   `exec_start netd1shot` → `start netd`. Keep stock loading, pinning,
   program attachment, init sockets and vendor hooks. Do not work around
   this with a late override after netd has started.
3. In an isolated signed test build, compare unmodified policy 0, the
   already reproduced stock policy-2 negative control, and the
   coordinated policy-2 candidate. Verify first boot and warm boot;
   trace `netd1shot` and both the first and repeated long-running netd
   starts, including zygote dependencies and service readiness. Compare
   effective/permitted/bounding caps, domains, SELinux denials/security
   negative controls, pinned-map identity, BPF readiness and a sysctl
   read-back before services start, after restarts, and after resume.
   Confirm arbitrary app map/program creation stays denied. No product
   grant to `netd1shot` if it does not need one; otherwise redo the
   least-privilege review before advancing.
4. Exercise actual Wi-Fi and mobile forwarding, Wi-Fi/cellular handoff,
   IPv4/IPv6, netd accounting and firewall rules, VPN/lockdown through
   a real peer, Wi-Fi and USB tether with a client and hardware offload
   where supported. Verify `network_stack`, `system_server`, and
   `system_suspend` separately; OTA CIL allows map creation but the
   live inventory did not prove another required create. Verify
   privileged OPlus BPF users, loader restart/startup semantics,
   KernelSU/SUSFS and Droidspaces. Do not infer these gates from pin FD
   counts or from a running-stack emulator test.

**External blocker:** matching platform init/netd/SELinux and tethering
APEX source/config for this firmware, or an OEM/authorized Mainline
update implementing the same compatible contract; for a local candidate,
a coordinated update signed by authorities accepted by this device for
both the APEX container and AVB payload. The verified Google container
certificate and payload AVB key are distinct. This init/policy design
also changes the authenticated system platform and therefore needs an
authorized OEM platform update under the matching `vbmeta_system`
chain, even if APEX delivery is through Mainline. Validate version/
rollback acceptance and CTS/VTS SELinux policy. No such build/signing path
is established. An unlocked bootloader or a lab key is not proof of
acceptance. See [Android APEX signing](https://source.android.com/docs/core/ota/apex).

## OPlus HBP: frozen reference, not a replacement

The byte-for-byte frozen reference diff is
`out/coloros17001201-warning-followup/public-hbp-reference.patch`,
SHA256 `de4842f8283840acb87f0e435571e675d92a4ff5ca701ec27978a7f0f11d22bd`.
It matches the three-file isolated worktree diff against `76b73bd515be1`;
the reverse apply check succeeds without changing the worktree. The
reference adds a real flexible array and serializes IRQ frame fill/copy
with transactional resize. It keeps the public wire offsets, checks
size and allocation before publication, and preserves the old buffer
on failure. Preserve the before/after ARM64 compile, DWARF/KCFI and
internal CRC receipts, and ASan/UBSan frame/bounds test with all 201
allocation-failure points. These are reference results, not stock
module compatibility.

| Stock module, identical copies in vendor_boot and vendor_dlkm | SHA256 |
|---|---|
| `oplus_hbp_core` (build ID `4d516877f163c9a30c35817e6f372947859e332a`) | `2603531d02e7e8796975bc58bdd6b849bd0d68682980cc9395e80d8586102a13` |
| `oplus_bsp_tp_hbp_goodix_gt99x6` | `87398fe57cd5885fbfdc5b1c24b063d244b4a5a384429f4789e8300790e3ed01` |
| `oplus_bsp_tp_hbp_syna_s3910` | `c510b1c7e662a8632e9660eea95776058e673ff46d65ae21d1c036d1e6e43644` |

Goodix requires `hbp_get_device_id` (CRC `0xf01ef47b`, KCFI
`0xc069e464`); Synaptics requires `hbp_dev_set_irq_status` (CRC
`0x7de15933`, KCFI `0xdfeae308`). Both are active dependencies in
normal and recovery plans, and both are absent in the public core.
`hbp_exception_cancel_deferred` (CRC `0xd272d446`, KCFI `0xe5c47d60`)
is also absent, without a direct active importer found. All 12 stock
exports, no export namespace, and the four chip variants, including
OPlus T-series and Focal, are preserved in `hbp-stock-contract.json`.
The shared 80-byte chip callback table, `get_frame` offset 40 and KCFI
`0xed529916` do not prove the rest of the private layout or locking.

In the observed coherent stock allocation, payload starts at offset
36, declared as a one-byte member; stock checks capacity as total
minus 39 and accepts payloads up to 8,192 bytes. The 2,048-byte case
requests 2,048 bytes of scratch and 2,087 bytes of destination via
`vzalloc` and ends at offset 2,084 exclusive. This is a FORTIFY
field-span warning, not a demonstrated allocation overflow. The
bounded-length proof does not establish IRQ/resize lifetime or chip
producer correctness. The diagnostic exists in the stock binary,
but its recurrence booting the OEM kernel is unverified. FORTIFY
remains active.

**External blocker:** exact .201 HBP core, Goodix and Synaptics source
and private headers, or an OEM-fixed matching dependency set. Need
the stock allocator/8-KiB behavior, IRQ/teardown lock and callback
contract, normal/recovery vendor SDK, generated headers and dependency
`Module.symvers`. Before packaging, verify both load paths, all imports
and export CRCs, KCFI/private structures, panel integration, frame
userspace ABI and real IRQ/resize/failure behavior. Then establish signed
firmware packaging and rollback for any affected partitions. No
replacement module or image was made.

## Physical stability matrix — unchanged policy and stock drivers

No reboot, radio toggle, service restart, synthetic input, charging
change, workload installation, or partition write was used. The
wireless ADB checkpoint around 09:05–09:08 CST on Nov 21, 2026
confirmed about 117 minutes of uptime, Android boot complete, user0
`RUNNING_UNLOCKED`, F2FS `inlinecrypt`, SELinux enforcing, and 689
live module entries. These are observations, not all subsystem passes.

| Area | Current read-only evidence | Functional qualification still needed |
|---|---|---|
| Wi-Fi | Associated and Android marked the Wi-Fi network validated; wireless ADB carried the diagnostics. | Sustained upload/download, reconnect and roaming. |
| Mobile data | LTE reported, mobile data enabled, cellular network reported validated; no app traffic forced onto it. | Real LTE/5G transfer, handoff and reconnect. |
| VPN | No active VPN confirmed in the checkpoint; emulator TUN-only result is separate. | Real VPN peer, DNS, and lockdown/reconnect. |
| Tethering | No active upstream in the checkpoint. Emulator hotspot-state result is separate. | Physical Wi-Fi and USB tether with a second client; DHCP, DNS, forwarding, offload and counters. |
| Charging | Battery health reported good, 100%, 26.9 °C; AC/USB/wireless all reported unplugged. | Plug/unplug and stable current/voltage and thermal behavior on supported chargers. |
| Touch | Stock HBP is retained; early FORTIFY warning is in the archived trace. | Tap, swipe, multitouch, edge input and repeated screen-off/wake/rotation. |
| Fingerprint | Fingerprint HAL provider reports sensor ID 1. | Physical repeated unlock/authentication. |
| Graphics | SurfaceFlinger reports Qualcomm Adreno 840, OpenGL ES 3.2. | Sustained GL/Vulkan, composition, frame pacing and GPU diagnostics. |
| Camera | Camera service exposes five devices, four listed as normal. | Preview, photo and video capture and playback, switching sensors. |
| USB | No connected host observed in the USB snapshot. | Cable data/ADB and OTG enumeration, transfer and clean eject. |
| Suspend/resume | Success counter rose 2,006 → 2,049; aggregate fail/abort counter rose 1,158 → 1,186 over ~53 s. An intermediate sample showed 940 freeze failures and zero failures in the resume-stage counters; latest recorded step then was freeze with `-16`. | Controlled unplugged screen-off soak, cause breakdown of aborted attempts, repeated wakes and device-specific checks. Counter increases alone do not classify all aborts. |
| Thermal | Android thermal severity 0 (no override) at idle. | Sustained load and post-suspend temperatures, throttling and recovery. |

**Additional stability disclosure:** the recent ring slice contained
120 `ktime_get+0xdc/0x118` WARN headers over about 17 seconds (not 120
independent root causes). An example has
`powermodel_cluster_suspend_handler [oplus_powermodel]` as the
caller via `dev_pm_genpd_suspend` and PSCI s2idle. The verified source
line is `WARN_ON(timekeeping_suspended)`. Suspend successes continued
to increase and the sampled resume-error counters were zero, but this
is not proof of power-management stability or stock-kernel equivalence.
Treat it as distinct from BPF and HBP and leave extended suspend open;
no diagnosis beyond the observed stack or source change was attempted.
The initial boot trace also reported a legacy `wpa_supplicant`
capability-format diagnostic. Neither diagnostic is suppressed.
No selected panic, Oops, fatal CFI/RCU, symbol/version rejection, verity
corruption, GPU fault/reset or UFS fatal/reset marker appeared in the
recent slice. It was a finite ring buffer; the retained boot traces,
not this later slice, establish the boot warnings.

**Handoff:** keep the currently installed boot-only .118 and its verified
rollback as the conditional baseline. BPF needs a trusted, coordinated
userspace/platform update and startup validation; HBP needs matching
vendor source/SDK or an OEM-fixed module set. Extended physical
qualification is still open. Nothing was rebuilt, installed, rebooted,
flashed, pushed or published in this closeout.

# ColorOS 17.0.1.201 / Linux 6.12.118 boot-warning follow-up

**No change to the phone.** The running OnePlus 15 CPH2747, slot A,
reported `6.12.118-android16-6-o-g76b73bd515be-4k`, source
`76b73bd515be1ed698c7df08ae29666eb3848b08`. Read-only `boot_a`
SHA256 matched the frozen tested image,
`8aecec4a35214a1ecb27e2170c28dcdd444542ac764bb3d84d3ab97447bfe16b`.
The archived stock rollback stayed
`e38045b1bdb0e51d71833206c848715cbc862f6fdc2039887cd7cd8248da5a4f`.
Audit data, compiled reference objects, and a reference source patch are
in `out/coloros17001201-warning-followup/`.

## A. eBPF / Spectre-BHB

The previously unverified writer is now verified on-device. The mounted
`com.android.tethering` APEX (version code `371021120`) supplies
`/apex/com.android.tethering/bin/netbpfload`, SHA256
`775d705134ac47146a536d31e76af3d439d0e29d48087bc6c64d04c102c4bed6`,
build ID `81ba38f9862aefc9d50234e9ef02c824`. It is byte-identical to the
executable in the .201 OTA. Live `netbpfload.35rc` overrides init's
placeholder service `bpfloader`. It runs as root with `SYS_ADMIN` and
`NET_ADMIN` (plus `CHOWN` and `SYSLOG`). The binary passes the sysctl
path and the literal zero to its file-writing helper. Its error path
exits on current kernels, before normal loading completes. Init
synchronously starts the loader and waits for `bpf.progs_loaded=1`
before starting the netd stages. Making zero unconditionally fail
is therefore a boot/network startup risk. The separate OTA
`/system/bin/bpfloader` continuation did not contain the sysctl path
as a literal; this is not a proof that no other writer exists.
[AOSP loader reference](https://android.googlesource.com/platform/packages/modules/Connectivity/+/5f020a5ab853acf2a5a3ece263de716987c3f0d0/bpf/loader/NetBpfLoad.cpp).

**The initial loader is privileged; downstream clients are a separate
contract.** With policy 2, BPF map/program creation still works with
the relevant capabilities; this loader does not need global
unprivileged BPF to obtain them. Live netd's effective mask
`0x74ef` lacks both `CAP_BPF` and `CAP_SYS_ADMIN`. The exact OTA
netd references pinned maps and xt_bpf programs used for accounting
and policy; OTA platform SELinux policy grants `map_create` to netd,
network_stack, system_server and system_suspend. Such permissions do
not prove which code paths actually create maps on .201, but they
rule out assuming all clients only use pre-created objects. The host
SETools build rejected the captured live policy's capability
encoding, so the OTA CIL is not presented as a complete live-policy
decision. VPN lockdown, traffic accounting/filters, tethering offload,
and mobile-data policy remain compatibility gates.

The exact offline hardening Image from `9075200417988` ran a new
ARM64 guest test. At sysctl 2, a privileged process created and
pinned an array; a process with no BPF capabilities could reopen,
lookup and update it with permitted file access, while new map creation
failed with `EPERM`. The original privileged filter test and
namespace/IPC/NAT/filesystem smoke checks also passed. This establishes
the Linux command boundary, not .201 Android service behavior.
No phone sysctl was changed and Android networking was **not tested at 2**.

The live kernel reports `Vulnerable: Unprivileged eBPF enabled` with
sysctl 0. In .118's `cpu_show_spectre_v2()`, this specific result
requires BHB state `SPECTRE_MITIGATED` and the permissive eBPF state;
it is not a failure to enable all ARM64 mitigations.
`CONFIG_MITIGATE_SPECTRE_BRANCH_HISTORY=y` is present. The kernel still
reports the residual BHB exposure when unprivileged program creation is
enabled. SELinux/seccomp can narrow which callers are exposed; neither
the warning nor this audit demonstrates a successful attack or that
every third-party app can load eBPF. The OEM-stock kernel contains the
same diagnostic and permissive default.

**Durable path:** preserve the reversible default 2, and change the
matching, properly signed firmware/APEX loader not to request zero
for this supported kernel/profile. Retain program loading, pinning,
attachment, init ordering, and privileged access. If actual
nonprivileged clients create objects, pre-provision them in a trusted
loader or move only the required operations behind a narrowly
authorized service; do not grant broad capabilities to apps. Before
deployment, trace successful startup, all requiring clients, network
accounting/firewall, Wi-Fi/LTE/5G, USB/Wi-Fi tethering (including
offload), VPN/lockdown, suspend, KernelSU/SUSFS and Droidspaces. Verify
the policy remains 2 after boot and service/connection changes.
Without a matched userspace build/signing path and those tests, this
is a plan, not a verified fix. No boot-time immutable sysctl, ignored
write error, patched binary, or warning suppression was used.

## B. HBP FORTIFY frame copy

The warning is in the stock .201 `oplus_hbp_core.ko` (identical
vendor_dlkm and vendor_boot copies; build ID
`4d516877f163c9a30c35817e6f372947859e332a`). The stock binary's
`frame_put+0x36c` raises the warning after checking a 2,048-byte
copy into the one-byte declared member. The retained call trace is
`hbp_irq_threaded_fn → frame_put → frame_write` (last function
inlined). The software defect is present in the shipping module.
Recurrence on an OEM-stock-kernel boot was not independently recorded.

- **Source:** chip-specific `hbp_dev_ops.get_frame(priv, queue->data,
  queue->data_size)` fills the `frame_queue` scratch payload. On
  success the IRQ passes the same pointer and configured length to
  `frame_put`. Stock `SET_FRAMESIZE` computes payload length as
  total size minus 39, checks positive growth and a limit of
  `0x2000` (8,192 payload bytes), and requests `vzalloc(payload)`.
- **Destination:** `frame_list.buf` points to a zeroed allocation
  requested via stock `vzalloc(total)` by `frame_init`. There are two
  16-byte timestamp unions, a 4-byte reason at offset 32, and payload
  at offset 36. The 40-byte legacy C layout declares `char data[1]`
  and includes three trailing pad bytes. The out-of-line storage and
  variable-size allocation establish that the payload is variable
  length, not intended to be limited to one byte.
- **Bounds:** stock saves the total and `total - 39` capacity.
  `frame_put` compares the supplied length to that capacity before
  copying into offset 36. For the observed 2,048-byte bounded copy,
  the destination's requested total must be at least **2,087 bytes**;
  for a coherent queue configured to a 2,048-byte payload, the scratch
  request is **2,048 bytes** and the frame allocation request is
  **2,087 bytes**. The copy ends at offset 2,084 (exclusive). These are
  inferred requested sizes, not a live allocation trace or allocator
  rounding measurement. The read path bounds `copy_to_user` by the
  stored total frame size.
- **Separate lifetime concern:** the stock ioctl publishes the length,
  frees the old scratch buffer, then reallocates before `frame_init`;
  the stock IRQ gets and copies the scratch pointer on a separate path.
  The public source shows the same unpaired synchronization and ignores
  `frame_write` errors. That is reason to repair replacement and error
  handling, not evidence that this specific warning was a heap
  overflow. FORTIFY correctly identified the declared-field crossing.

The three reference files match the current [OnePlus 15 public source
tip](https://github.com/OnePlusOSS/android_kernel_modules_and_devicetree_oneplus_sm8850/tree/4c03af1d6d462b9573a7a9f85b983c6bae49497a/vendor/oplus/kernel/tp/hbp/hbp);
the four public SM8850 branch tips have identical `hbp_frame.c/.h`.
The OnePlus 15 file history points to the PLK110 16.0.0.204 import
`7122d08fa2f3dc5e6f3da61fde10b596d7b61aea`. No published OEM fix was
found in that scope. This is **not the stock .201 source**: the public
code uses `kzalloc` and a `PAGE_SIZE` (4 KiB in this build) payload
cap, whereas stock uses `vzalloc` and allows 8 KiB.

### Reference repair and deployment gate

Isolated worktree `out/coloros17-612118-hbp-reference-repair`, branch
`experiment/coloros17-612118-hbp-reference-repair`, holds a source
prototype, not a qualified module. It uses a true flexible array and
keeps the 36-byte payload offset, 40-byte base size, eight-byte alignment
and legacy 39-byte subtraction. It checks arithmetic before allocating,
checks the payload and backing bounds before copying, and propagates a
failed frame enqueue. Resize allocates the new scratch buffer and frame
pool before publication and swaps them under the same mutex held across
IRQ fill and copy. Allocation failure preserves the old pool. The
prototype intentionally retains the **public** allocator and frame-size
limit; a stock .201 port must preserve the actual contract, including
the stock 8 KiB limit, allocation paths, error handling, callback lock
ordering and teardown.

- Both affected public translation units compiled before/after with the
  frozen .118 generated headers and the production Clang 19 ARM64 flags,
  including FORTIFY, strict flexible arrays and KCFI. The field-specific
  WARN is absent from the fixed reference object. FORTIFY remains on.
- DWARF checks show unchanged `frame_list`, `frame_queue` and
  `hbp_device` sizes/member positions, and unchanged wire offsets.
  Five actual compiled function-entry KCFI prefixes match, including
  the IRQ and ioctl handlers. Three internal frame signature CRC
  diagnostics match; these functions are not exported module symbols.
- Host ASan/UBSan test of the edited frame code passed the legacy
  2,048-byte round-trip, exact bounds, rejection before queueing and
  all 201 scratch/pool allocation-failure points. It does not validate a
  real panel's callbacks or IRQ lock ordering.
- **Actual stock replacement fails the export inventory gate.** The
  public source does not provide .201's `hbp_get_device_id`
  (`0xf01ef47b`, imported by stock Goodix GT99x6) or
  `hbp_dev_set_irq_status` (`0x7de15933`, imported by stock Synaptics
  S3910), in both normal and recovery load plans. It also lacks stock
  `hbp_exception_cancel_deferred`. No force-load, dummy exports or
  private-layout assumptions can make this a verified replacement.

To finish, obtain .201-matching HBP core and chip source (including
the three missing implementations and private headers), the actual
normal/recovery build flags and full dependency `Module.symvers`, or
an OEM-supplied fixed matched module set. Port the bounds fix there
and rerun the whole retained-module oracle, callback KCFI and firmware/
HAL tests. Any eventual matched vendor_dlkm/AVB package needs a verified
parent/rollback contract and independent checks of the stock recovery
provider. No incompatible module or flashable image was assembled.

## C. Compatibility disposition

The corrected independent ColorOS comparison was replayed with exit
**0**, and the preserved stock result remains **1,031 active compatible,
38 dormant**, with zero CRC/provider/namespace failures for the
unchanged artifact. Enforced Kleaf GKI remains **FAIL / exit 4**, with
the existing 76 blocks; no ABI reference, STG extractor, kernel,
`arm_smmu` contract, or retained module was modified. No combined
build, warning-free boot, or device qualification is claimed for the
source prototype. Linux .118 security work, KernelSU/SUSFS, Droidspaces
and the stock Qualcomm graphics and driver population remain on the
existing image. Nothing was flashed, rebooted, pushed or published.

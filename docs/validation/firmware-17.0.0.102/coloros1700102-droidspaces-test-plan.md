# Droidspaces: controlled ColorOS 17 physical test plan

Status: full container-functional matrix NOT PERFORMED. An initial separately
authorized kernel flash, Android/FBE and live checker test passed; see the
[physical record](coloros1700102-droidspaces-canary-20261103.md). This plan by
itself does not authorize flashing, installing apps/containers, changing slot
state or touching userdata/metadata partitions.

## Frozen offline-qualified candidate

- Branch `experiment/coloros17-linux115-droidspaces`, base
  `d6552d3625e0656aeac303b878347bef208b4315`, uncommitted common snapshot
  `6d074b67a27e4ccd1582be8178ab0bf8284870aa`.
- Boot path:
  `/home/travis/Android/stock_kernel/out/droidspaces-build/candidate/boot.img`.
- Boot SHA256:
  `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc`.
- Image SHA256:
  `a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1`.
- Release: `6.12.115-android16-6-o-gd6552d3625e0-4k`.
- Same folder contains `manifest.json`, `boot.img.sha256`, `SHA256SUMS`.
  Image is 100,663,296 bytes, v4 boot header, empty cmdline/ramdisk, same
  qualified development boot signer and outer firmware contract.
- Static stock-module/provider checks pass. Enforced GKI remains FAIL: all 44
  previous owner differences unchanged plus one reviewed C-private IPC-padding
  addition. ARM64 VM passes the actual checker and functional IPC/storage tests.
  Neither result establishes real Android/container startup.

See [the full audit](../../features/droidspaces-coloros17.md) for exact source
changes, build outputs, signing/module evidence and SUSFS limitations.

## Safety and exact artifact gate

1. Obtain explicit approval for the frozen boot image and test cycle. Check live
   OnePlus 15 identity, ColorOS `CPH2745_17.0.0.102(EX01)`, running kernel, active
   slot, bootloader state, Android/FBE health and update/snapshot status.
2. Verify the candidate manifest, image SHA256, boot partition capacity and AVB
   metadata. Preserve exact qualified boot bytes and full partition readback.
   Known qualified KSU/SUSFS-uname rollback boot SHA256:
   `3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`.
3. Only the explicitly approved active-slot boot partition may change. Do not
   change system_dlkm, vendor_dlkm, vendor_boot, init_boot, vbmeta, recovery,
   slots, dynamic partitions, AVB keys, rollback indexes or encryption state.
4. Use the existing validated write/readback workflow. Never erase/format data
   or metadata. Restore the exact qualified boot through the validated recovery
   route if early Android/FBE/module checks fail; capture safe failure evidence
   first. Do not repeatedly boot a failing candidate.

## Android first: mandatory gates

- Verify boot readback hash and source/build identity; verify `/proc/config.gz`
  against the candidate effective config, and record `/proc/cmdline`/bootconfig.
- Confirm `sys.boot_completed=1`, user0 `RUNNING_UNLOCKED`, PIN and fingerprint
  unlock, CE/DE access, KeyMint/keystore/vold health, `/data` F2FS+inlinecrypt.
- Confirm original stock module partitions, module hashes/provider identities,
  signing policy and dm-verity remain unchanged. Stock KGSL/GEAS, Bluetooth,
  WLAN, RMNET, camera and audio remain the production providers.
- Check panic/Oops/BUG/WARN, KCFI, RCU, module/CRC/namespace errors and watchdogs.
  No new unexplained errors may be treated as PASS.

## Runtime checker and controlled policy

- Review the pinned official Droidspaces source/app/binary and use trusted
  releases. Grant only the necessary KernelSU app/root access.
- Record SUSFS state. Official Droidspaces calls SUSFS unsupported; its global
  **Hide sus mounts for all processes** setting must be off for an attempt.
  No automatic SUSFS setting changes are part of this kernel task. Initially
  use the real uname so version checks are meaningful.
- Run `droidspaces check --json` through its documented root/daemon context.
  Save the complete JSON, stderr and logs. **Exit zero is not PASS**; require
  `requirements_met=1`. The checker transiently creates/deletes bridge/veth
  interfaces and can write its cache: review/authorize that runtime action.
- Record inherited seccomp mode, SELinux denials, mount/cgroup ownership and
  ns identities. Diagnose context-specific denial; do not globally disable
  SELinux/seccomp, weaken KSU, or add broad permissive rules.

## Distribution functional matrix

Use independently verified trusted Ubuntu and Debian ARM64 rootfs checksums,
separate disposable container directories/ext4 loop images, sufficient free
space, and a narrowly scoped backing directory. Do not pass host raw block
devices, phone credentials, GPU devices, USB, camera or all `/dev` by default.
Leave both `--hw-access` and `--gpu` off: upstream's hardware mode exposes the
shared devtmpfs, and its GPU mode also mirrors DMA and Android Binder devices.
Use normal private-tmpfs `/dev`; Binder/Waydroid isolation is not qualified by
this kernel task. Record that no such devices were passed into the container.
Fedora/systemd and Alpine/OpenRC should be tested separately before claiming
those distributions are supported.

| Test | Evidence required | Status |
|---|---|---|
| Create/start each container | runtime logs, init PID and successful shell | NOT TESTED |
| systemd / OpenRC init | healthy target/service, no boot loop | NOT TESTED |
| PID/mount/IPC/UTS/user/net isolation | compare ns IDs and visible PIDs/mounts, scoped IPC test | NOT TESTED |
| cgroup v1/v2 | actual Android hierarchy, delegated memory/CPU controls, cleanup | NOT TESTED |
| proc/sys/devpts/devtmpfs | actual guest mounts and working PTYs | NOT TESTED |
| Networking | bridge/veth, IPv4 NAT, DNS lookup, HTTPS outbound, cleanup | NOT TESTED |
| Filesystems/persistence | ext4/overlay scoped create/write/read/delete, stop/start persistence | NOT TESTED |
| Stop/restart | init terminates, no leaked mounts/loop devices/veth/cgroups | NOT TESTED |
| Suspend/resume | USB-disconnected 10–15 minute idle, wake/unlock/reconnect, logs | NOT TESTED |
| Android regressions | WLAN/BT/LTE, camera, audio, fingerprint, USB OTG, charging | NOT TESTED |

Do not call delegated controllers functional merely because `/proc/cgroups`
lists them. PIDs limits, v1 device controller, MACVLAN, NAT66 and nftables are
not enabled by this minimal experiment. Nested Docker/Podman is a separate
optional compatibility/security project, not implied by base runtime success.

## Optional hardware/desktop scope

Mesa Turnip, Termux:X11, nested container engines and USB passthrough require
separate authorization and trusted workloads with minimal device access. Do
not grant privileged hardware access to untrusted rootfs images. Containers
share the host kernel and are not a VM isolation guarantee.

Record every result against exact boot/Image/source/config hashes. Keep physical
NOT TESTED distinct from ARM64 VM PASS, stock-module static PASS and the honest
enforced GKI comparison result. Stable release inclusion requires this matrix
and preservation of all existing ColorOS physical gates.

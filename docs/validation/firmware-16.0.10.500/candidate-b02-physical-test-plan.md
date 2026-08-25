# Candidate B02 physical-test plan

## Isolation and payload identity

This plan is for later authorization only. Candidate B02 has not been flashed.
The only permitted write is `boot_a` on OOS 16.0.10.500 slot `_a`.

| Item | Required identity |
|---|---|
| Candidate boot | `dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07` |
| B01 rollback boot | `2646a4d773ac6360cf981c4148fd37b128e8f0cd53abd07418a6807641e9d091` |
| Stock system_dlkm | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| Stock vendor_dlkm | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| Stock vendor_boot | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Do not supply or write system_dlkm, vendor_dlkm, vendor_boot, VBMeta, or slot
metadata.

## Preflight, backup and write

1. In Android, verify slot `_a`, completed boot, B01 kernel identity, existing
   user0 `RUNNING_UNLOCKED`, 6 GHz WLAN, registered cellular/RMNET dual stack,
   routes, IP/DNS, and current stock DLKM hashes.
2. Reverify the candidate file SHA-256. Do not rebuild it.
3. Enter the hardened TWRP environment. Require CPH2747/Canoe identity, slot
   `_a`, snapshot/update state `none`, partition capacity, and a decrypted or
   recovery-tmpfs plus host backup destination.
4. Make a full `boot_a` backup. Require source size/hash equals backup
   size/hash and preserve the independent exact B01 rollback artifact.
5. Run a boot-only dry run. Require the device, slot, capacity, candidate hash,
   and backup gates to pass.
6. Write only `boot_a`, read the whole partition back, and require exact B02
   SHA-256 before rebooting.

## Core physical gates

- two clean Android boots;
- existing encrypted user0 `RUNNING_UNLOCKED` on both boots;
- `init_user0_failed`, Rescue Party redirect, and prolonged-logo stall not
  observed;
- truthful kernel release
  `6.12.23-android16-5-o-gab336ec00b4b-4k`;
- 6 GHz / 6135 MHz WPA3, Internet and Wi-Fi off/on;
- LTE HOME/IN_SERVICE, active RMNET IPv4/IPv6, routes, numeric IP and DNS;
- Bluetooth off/on plus existing reconnect when available;
- NFC service, Wallet/HCE/eSE state;
- camera open/capture and ringtone/media playback;
- normal UI and ADB/USB enumeration;
- deep idle/wake 5/5 with WLAN/cellular recovery.

## BPF-specific coverage

Use existing Android facilities only; do not patch or instrument the kernel.

1. Verify `/sys/fs/bpf` or the firmware-native bpffs mount and inventory the
   available programs/maps.
2. Confirm `bpfloader`, `netd`, TrafficController and related Android network
   accounting/policy services remain healthy.
3. Capture available BPF map/program summaries and kernel/logcat BPF messages
   before and after stress. Absence of `bpftool` is not itself a failure.
4. Run at least 50 safe app launch/close and foreground/background traffic
   operations while alternating Wi-Fi and cellular connectivity.
5. Exercise UID/network-policy churn through normal Settings and Android
   service commands where supported; do not disrupt critical services.
6. Run sustained traffic on both Wi-Fi and cellular and verify network stats,
   accounting, firewall policy, handoff, IP and DNS remain functional.
7. If an already available trusted userspace test can create a four-byte
   CGROUP_STORAGE value and copy it into a per-CPU map, run it and preserve the
   result. Do not install unreviewed privileged binaries during qualification.

The exact vulnerable trigger is independently classified:

- `OBSERVED` only with evidence of the non-eight-byte-aligned
  CGROUP_STORAGE-to-per-CPU-map copy; or
- `NOT OBSERVED` if only normal Android BPF compatibility was exercised.

A compatibility PASS does not by itself prove that the specific OOB condition
was triggered.

## Error scan and acceptance

Capture full `dmesg` and `logcat -b all`. Scan for BPF loader/verifier/map
failures and for out-of-bounds, KASAN, UBSAN, use-after-free, refcount,
Oops, BUG, panic, Call trace, IOMMU/SMMU, Unknown symbol, CRC/MODVERSION,
vermagic, signature, and protected-export failures. Compare warnings with B01.

PASS requires 2/2 boots, unlocked existing user0, all core subsystem gates,
BPF/netd compatibility, deep idle 5/5, and no new critical kernel/module/BPF
failure. Report trigger coverage separately.

At the first critical failure, stop and restore exact B01 boot. Verify full
read-back SHA-256 before rebooting. Do not select or test B03.

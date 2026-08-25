# Candidate B02: BPF per-CPU map bounds static validation

## Decision

**READY — CANDIDATE B02 PHYSICAL TEST**

Candidate B02 cumulatively contains physically qualified B01 Binder lifetime
hardening plus one new coherent change: the BPF per-CPU map value-copy bounds
fix. No ACK advancement, DLKM adaptation, built-in conversion, device-tree
change, or unrelated cleanup is included. No B02 payload has been flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B01 parent | `b9b15f8a22a906786729cf830c47d0c6cd237e9a` |
| B02 branch | `experiment/oos1610500-custom-r53-b02` |
| B02 runtime source | `ab336ec00b4bf6a86fde5ba682852fefa06de0c8` |
| Kernel release | `6.12.23-android16-5-o-gab336ec00b4b-4k` |
| Intended B02 runtime paths | `kernel_platform/common/kernel/bpf/hashtab.c` |
| Unexpected runtime groups | 0 |

B01 is an ancestor of B02. The B01 Binder hardening remains present and is not
duplicated or modified.

## Value and reachability

`pcpu_init_value()` previously used a rounded long-copy helper for the
current-CPU value. A non-eight-byte-aligned source, such as a four-byte
`CGROUP_STORAGE` map value, could therefore cause an out-of-bounds read when
copied into a per-CPU map. B02 uses the declared-size-aware helper.

The fix has real runtime value and is not merely build metadata. The current
configuration enables BPF, BPF syscalls, cgroup BPF, and BPF events. The r53
source was confirmed pre-fix; the direct patch applies without prerequisite
project customizations.

## Build, ABI and KMI

These targets passed under B01's exact Zstd-DWARF configuration:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All five ABI reports and the ABI error file are empty; ABI diff exit code is
zero. KMI strict-mode and KMI symbol-list violation checks passed. The B02
`.config` and `Module.symvers` are byte-identical to B01.

| Artifact | SHA-256 |
|---|---|
| Image | `098f4c2e0ca1b27ec0e238919aa3080b116b3686f73a751cf9e1a39634b035e5` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `ee46b9564435c842bc850c97359bc99388fb0afcae45785d6f67a5b3ec31bfc5` |
| vmlinux | `ecc7d33e1ebe8861ee9ce78259895e34f4cd0cd023485b1ab3ed6ce690c56269` |

## FBE and existing-user0 contract

The source diff is empty across fscrypt, F2FS, ext4, block/blk-crypto,
device-mapper, keyrings, UFS, SCSI, and ICE-sensitive paths. No encryption or
storage config changes. Static FBE/storage runtime delta relative to physically
qualified B01: **0 / PASS**.

Physical qualification must still prove existing user0 reaches
`RUNNING_UNLOCKED` on two boots and that `init_user0_failed` is not observed.

## Retained stock module contract

| Gate | Result |
|---|---:|
| Physical module files checked | 1,020 |
| Active compatible module files | 984 |
| Dormant module files | 36 |
| Import/CRC edges matched | 57,216 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature/trust failures | 0 |
| Structural-provider failures | 0 |
| Static result | PASS |

The exact native stock payload contract remains:

- system_dlkm SHA-256:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- vendor_dlkm SHA-256:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- vendor_boot SHA-256:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`
- system `modules.load`: 82 entries
- `wwan.ko`: entry 28
- unexpected DLKM changes: 0

## Boot-only payload

The candidate uses the same proven OOS 16.0.10.500 stock-container method as
Candidate A and B01. Repacking the B01 kernel first reproduced the exact B01
boot hash, validating the procedure. For B02, the 4,096-byte boot header and
17,920-byte non-kernel tail are byte-identical to stock; the embedded kernel is
byte-identical to the accepted Image.

AVB footer/hash verification passes with the established unlocked-device GKI
development key. The stock OS version, security patch, fingerprint property,
partition layout, and salt are retained. No OEM private key is claimed.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | stock current-firmware container, kernel replaced | `dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b02-final/boot.img
```

## Physical-test requirement

Write `boot_a` only after preflight, verified backup, dry run, exact candidate
hash, and full read-back. Roll back to exact B01 boot SHA-256
`2646a4d773ac6360cf981c4148fd37b128e8f0cd53abd07418a6807641e9d091`
on the first critical failure.

In addition to two boots, user0/FBE, WLAN, cellular, BT, NFC, camera, audio,
idle, USB and kernel-error gates, validate stock Android BPF health: bpfloader,
netd/TrafficController, mounted bpffs, expected programs/maps, UID/network
policy churn, app traffic across Wi-Fi and cellular, and BPF verifier/OOB
error scans. If the exact four-byte CGROUP_STORAGE-to-per-CPU-map condition
cannot be generated with existing safe userspace tools, report the fix trigger
as **NOT OBSERVED** rather than claiming behavioral demonstration.

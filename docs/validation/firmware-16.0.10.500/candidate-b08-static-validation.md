# Candidate B08: netfilter quota2 lifetime static validation

## Decision

**READY — CANDIDATE B08 PHYSICAL TEST**

Candidate B08 cumulatively contains physically qualified B01 through B07 plus
one netfilter quota2 shared-counter lifetime correction. It does not advance
ACK, modify a DLKM, alter vendor_boot/VBMeta/slot state, change configuration,
or cross an external provider boundary. It has not been flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B07 parent | `c2629b3fd92bc913921be48c60dc4b1ad7c68b94` |
| B07 tested runtime source | `969639e8ca81ec5048338b4366cf17de28941029` |
| B07 tested boot | `1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1` |
| B08 branch | `experiment/oos1610500-custom-r53-b08` |
| B08 runtime source | `338c09465853ddbccde4861e14f8c8fa2f24342e` |
| Kernel release | `6.12.23-android16-5-o-g338c09465853-4k` |
| Unexpected runtime groups | 0 |

B07 is an ancestor of B08. The runtime diff contains only the audited
`q2_get_counter()` hunk in `net/netfilter/xt_quota2.c`.

## Build, ABI, KMI, config and FBE

The final stamped Zstd-DWARF build passed these targets:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

The successful invocation explicitly preserved the B07 Zstd-DWARF build mode.
All ABI reports and the ABI error file are empty, and the ABI diff exit status
is zero. B08 `.config` and `Module.symvers` are byte-identical to B07. The
B07-to-B08 source diff is empty across fscrypt, F2FS, ext4, block/blk-crypto,
device mapper, keyrings, SELinux, UFS, SCSI, ICE and crypto.

| Artifact | SHA-256 |
|---|---|
| Image | `8497f45612fc4e5314a7bebd837cea7e74de2664c177a67c6660d24c337bc010` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `9c33f22b748c982be163a9367ee1b28f5e4c1936f8675908a9ccef26e9433d2f` |
| vmlinux | `1f9613c0f0ac60efda3920ebcb45ac4c5a350d62a3f12a482f35497a0cce7c6a` |

FBE/storage runtime delta: **0 / PASS**.

## Firmware-native module and trust contract

Candidate A through B08 intentionally retain the firmware-native r53 trust
configuration and exact stock current-firmware DLKMs. This is not the legacy
16.0.9.400 controlled-v1 project-signing contract. The stock 16.0.10.500
certificate is present in B07 and B08 vmlinux, and the retained-module audit
classifies all 103 signed system modules as `STOCK_TRUSTED` and all 917
vendor/vendor_boot modules under their unchanged unsigned contract.

| Gate | Result |
|---|---:|
| Physical module files checked | 1,020 |
| Active compatible | 984 |
| Dormant | 36 |
| Import/CRC edges matched | 57,216 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature/trust failures | 0 |
| Structural-provider failures | 0 |
| Static result | PASS |

The immutable current-firmware payload contract remains:

- system_dlkm: `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`;
- vendor_dlkm: `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`;
- vendor_boot: `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`;
- stock system `modules.load`: 82 entries;
- `wwan.ko`: entry 28;
- replacement DLKM/vendor_boot images produced: 0.

## Boot-only payload

The packaging gate first reproduced the physically tested B07 boot image
byte-for-byte. B08 uses the same 16.0.10.500 header, zero-length ramdisk,
preserved embedded 16 KiB GKI-signature tail, partition geometry, descriptor
properties, salt and established unlocked-device development AVB method.

The 4,096-byte header and complete 16,384-byte non-kernel tail are byte-identical
to B07. The embedded kernel is byte-identical to B08 Image; outer AVB hash and
signature verification pass. The development key is not presented as an OEM
private key or OEM signature.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | current-firmware container, kernel replaced | `66046855b5d44aae821336c6499b3b741866beb82b8413fbeaf7769b6b324c1a` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b08-final/boot.img
```

## Boot-only phase assessment after B08

The remaining 27 unqualified project groups are partitioned without overlap:

| Bucket | Count | Boundary |
|---|---:|---|
| remaining high-value boot-only groups | 2 | SFQ internal fixes; UCSI validation with a firmware-facing risk gate |
| remaining medium/low-value boot-only groups | 4 | ADIOS, global workqueue policy, optional TCP/qdisc feature policy, HID without matching hardware |
| deferred to controlled-DLKM phase | 12 | trust/KMI ownership, built-in conversions, module security work and vendor subsystem/platform sources |
| high-risk or not worth currently | 9 | broad OEM/SKB semantics, FBE/storage/filesystem, pKVM and inactive paths |

B08 does not select B09. SFQ is attached but idle in the B07 evidence, and UCSI
crosses a retained QTI GLINK/firmware provider boundary. The boot-only phase is
not declared complete solely by count, but any continuation must first improve
runtime reachability or justify the higher boundary risk.

Physical validation is required before a B08 qualification freeze. The exact
boot-only plan is in `candidate-b08-physical-test-plan.md`.

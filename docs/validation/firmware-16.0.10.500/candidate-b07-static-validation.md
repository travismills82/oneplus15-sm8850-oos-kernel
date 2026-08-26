# Candidate B07: IPv6 MLD skb lifetime static validation

## Decision

**READY — CANDIDATE B07 PHYSICAL TEST**

Candidate B07 cumulatively contains physically qualified B01 through B06 plus
one IPv6 MLD query skb-lifetime correction. It does not advance ACK, modify a
DLKM, alter vendor_boot/VBMeta/slot state, change configuration, or cross an
external module-provider boundary. It has not been flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B06 parent | `67ddf38649ea9b60fe30ec7be2a352620fe8ea2f` |
| B06 tested runtime source | `aad7cdfe6542f3fb51d751236bbacde59e2d9b93` |
| B06 tested boot | `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825` |
| B07 branch | `experiment/oos1610500-custom-r53-b07` |
| B07 runtime source | `969639e8ca81ec5048338b4366cf17de28941029` |
| Kernel release | `6.12.23-android16-5-o-g969639e8ca81-4k` |
| Unexpected runtime groups | 0 |

The B06 qualified head is an ancestor of B07. The runtime diff contains only
the audited eight-line hunk in `net/ipv6/mcast.c`.

## Build, ABI, KMI, config, and FBE

The stamped Zstd-DWARF build passed:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All ABI reports and the ABI error file are empty, and the ABI diff exit status
is zero. B07 `.config` and `Module.symvers` are byte-identical to B06. The
B06-to-B07 source diff is empty across fscrypt, F2FS, ext4, block/blk-crypto,
device mapper, keyrings, SELinux, UFS, SCSI, ICE and crypto.

| Artifact | SHA-256 |
|---|---|
| Image | `717b3615ea0dbfe92a3f8efab974ec549f98d6b2b1874b2afb15bfa350d42f4a` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `d7622b4b4ea3ab9a700df6b90ff30c2da81fc323c7b582c79653f2a1c3f1fec3` |
| vmlinux | `05da2f065b251b08587ee7ce6f92c1643c45ccffad96beb0157eb5f224265495` |

FBE/storage runtime delta: **0 / PASS**.

## Retained current-firmware module contract

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

The packaging gate first reproduced the physically tested B06 boot image
byte-for-byte. B07 uses the same 16.0.10.500 header, zero-length ramdisk,
preserved 17,920-byte non-kernel tail, partition geometry, descriptor
properties, salt and established unlocked-device development AVB method.

The 4,096-byte header and 17,920-byte non-kernel tail are byte-identical to
B06. The embedded kernel is byte-identical to B07 `Image`; outer AVB hash and
signature verification pass. The development key is not presented as an OEM
private key or OEM signature.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | current-firmware container, kernel replaced | `1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b07-final/boot.img
```

Physical validation is required before any qualification freeze. The exact
boot-only plan is in `candidate-b07-physical-test-plan.md`.

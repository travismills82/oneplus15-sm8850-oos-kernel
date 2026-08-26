# Candidate B06: AF_UNIX GC/SCC hardening static validation

## Decision

**READY — CANDIDATE B06 PHYSICAL TEST**

Candidate B06 cumulatively contains physically qualified B01 through B05 plus
one coherent two-commit AF_UNIX GC/SCC lifetime-hardening series. It does not
advance ACK, modify any DLKM, alter vendor_boot/VBMeta/slot state, change
configuration, or cross an external provider boundary. It has not been
flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B05 parent | `e17fd9021ded0d11c48ef5761df59ce931c6c0b5` |
| B05 tested runtime source | `515d73a3d5f436bb3b67d36ef1be44fafd22e0ae` |
| B05 tested boot | `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8` |
| B06 branch | `experiment/oos1610500-custom-r53-b06` |
| B06 runtime source | `aad7cdfe6542f3fb51d751236bbacde59e2d9b93` |
| Kernel release | `6.12.23-android16-5-o-gaad7cdfe6542-4k` |
| Unexpected runtime groups | 0 |

The B05 qualified head is an ancestor of B06.

## Build, ABI, KMI, config, and FBE

The stamped Zstd-DWARF build profile used by B05 passed:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All ABI reports and the ABI error file are empty; the ABI diff exit status is
zero. B06 `.config` and `Module.symvers` are byte-identical to B05. The
B05-to-B06 source diff is empty across fscrypt, F2FS, ext4, block/blk-crypto,
device mapper, keyrings, SELinux, UFS, SCSI, ICE, and crypto.

| Artifact | SHA-256 |
|---|---|
| Image | `a28ccaa6ed636c8a1e2b996154c211bf83dbd5f97255fae899da30e34786abf5` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `a89d05f70c4ccf72a611014c6a10f05107d3015204ce97a4b73e9819c8ac882f` |
| vmlinux | `18d68a3236f5fbd243f3b2477a8a7f899f293a5afec684a0ba38b2b5d966d416` |

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

The packaging gate reproduced the physically tested B05 boot image
byte-for-byte before creating B06. B06 uses the same 16.0.10.500 header,
zero-length ramdisk, 16 KiB preserved firmware-native GKI signature tail,
partition geometry, descriptor properties, salt, and established unlocked-
device development AVB method. The preserved inner GKI tail is not presented
as a new signature for B06 and no OEM private key is claimed.

The 4,096-byte header and 17,920-byte non-kernel tail are byte-identical to
B05. The embedded kernel is byte-identical to the B06 `Image`; outer AVB hash
and signature verification pass.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | current-firmware container, kernel replaced | `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b06-final/boot.img
```

The exact future boot-only qualification is in
`candidate-b06-physical-test-plan.md`. Compatibility and exact race coverage
remain separate outcomes.

# ENC-S01 static validation

Validation date: 2026-09-10 (America/Chicago)

## Result

`READY — ENC-S01 PHYSICAL FBE CANARY`

This is a boot-only, pre-physical-test result for the fscrypt
multi-data-unit-size key-cache correction.  It remains naturally on Linux
6.12.35 and Android KMI generation 5.  It does not authorize ENC-S02 or an ACK
6.12.36 update before the physical decryption oracle passes.

| Item | Value |
|---|---|
| Starting `main` | `14e983c09ff6b01c6f13eae2b6a00aadac9191e1` |
| Development branch | `experiment/oos1610500-ack-6.12.35-encryption-hardening` |
| Validated source | `ad4ab6465d9925abe369e20d001bd2c765417da1` |
| Authoritative backport | `dbb2a6c1df7fd018aa74353762e6b4659b6a4593` |
| OOS generation-5 adaptation | `ad4ab6465d9925abe369e20d001bd2c765417da1` |
| Kernel release | `6.12.35-android16-5-o-gad4ab6465d99-4k` |

## Build and ABI/KMI

The following commands completed successfully from `kernel_platform/`:

```text
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel build \
  //common:kernel_aarch64_abi \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //common:kernel_aarch64_abi_diff
tools/bazel run //common:kernel_aarch64_abi_dist -- \
  --destdir=out/enc-s01-abi-dist
tools/bazel build --kbuild_symtypes //common:kernel_aarch64
tools/bazel build //soc-repo:canoe_perf_abi
```

| Gate | Result |
|---|---|
| Canoe distribution | PASS |
| Common kernel | PASS |
| Enforced common ABI | PASS; empty diff |
| Strict KMI symbol list | PASS |
| Symtypes | PASS |
| Canoe device-module-inclusive ABI dump | PASS |
| ABI reference change | NONE |
| Kconfig change | NONE |

The Canoe ABI target is a device-module-inclusive dump, not an enforced device
ABI comparison.  The enforced common ABI target remained enabled and produced
an empty diff.

## Real structure-layout proof

Plain application of the newer fscrypt fix changed the ABI-represented
`struct fscrypt_master_key` from 872 to 360 bytes.  The downstream adaptation
preserves the qualified generation-5 representation rather than masking it.

| Member | Qualified offset | ENC-S01 offset | Result |
|---|---:|---:|---|
| `mk_direct_keys` | 320 | 320 | MATCH |
| `mk_iv_ino_lblk_64_keys` | 496 | 496 | MATCH |
| `mk_iv_ino_lblk_32_keys` | 672 | 672 | MATCH |
| `mk_ino_hash_key` | 848 | 848 | MATCH |
| `mk_ino_hash_key_initialized` | 864 | 864 | MATCH |
| `mk_logged_hw_wrapped_keys` | 865 | 865 | MATCH |
| `sizeof(struct fscrypt_master_key)` | 872 | 872 | MATCH |

The first bytes of the obsolete private `mk_direct_keys` storage hold the new
mode-key list head.  Compile-time size and alignment assertions guard that use.
No matching external OnePlus, Qualcomm, or common-module source directly
accesses the three private arrays.  The functional implementation still keys
prepared keys by HKDF context, crypto mode, data-unit-size bits, and
inline-versus-software implementation.

## Configuration and provider identity

The final generated configuration is byte-identical to the Phase 1 physically
tested configuration:

```text
SHA-256 166e1db5f0a5e5e33b801413d3f5255f8f92dcf8e210317d24d27cb3389897d4
```

All required F2FS, fscrypt, blk-inline-encryption, device-mapper, UFS crypto,
module-signing, MODVERSIONS, and GENDWARFKSYMS settings remain enabled exactly
as qualified.

The final `Module.symvers` is byte-identical to the Phase 1 provider oracle:

| Item | Value |
|---|---|
| Records | 9,356 |
| Size | 575,272 bytes |
| SHA-256 | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |

The earlier disk-space cleanup removed the extracted stock module binary
roots.  This validation therefore replays the previously accepted 1,020-module
result through exact provider identity; it does not claim a newly extracted
binary rescan.  The replayed contract is 984 active plus 36 dormant modules,
57,216 import/CRC edges, zero blockers, zero CRC mismatches, zero unresolved
imports, zero protected-export or signature failures, and `rust_binder.ko`
166/166.  A fresh extraction/rescan from the connected stock device may be
performed during physical preflight without modifying its supporting
partitions.

## Signing

The build graph retained the pinned hermetic OpenSSL from
`platform/prebuilts/build-tools` commit
`951b0e9b947327fe485b2553faef2ed34f8e148d`.

| Check | Result |
|---|---|
| Generated GKI modules | 103 |
| SHA-512 signatures | 103/103 PASS |
| PKCS#7 module-signature trailer | 103/103 PASS |
| Expected signer | 103/103 PASS |
| Host OpenSSL fallback | NO |
| SHA-1 fallback | NO |

## Boot-only artifact

| Item | Value |
|---|---|
| Path | `out/oos1610500-ack-6.12.35-encryption-hardening-enc-s01/boot.img` |
| Size | 100,663,296 bytes |
| SHA-256 | `a4099be9d60f2d81268d64ab9e1d38efdf3a8a6c15638ba5d60f3384f423cf3d` |
| Embedded Image size | 39,959,040 bytes |
| Embedded Image SHA-256 | `e4977a2531486ee3bc7b49330c787775b9db0256b668b4fa68ca87331f02fe9e` |
| Kernel release | `6.12.35-android16-5-o-gad4ab6465d99-4k` |
| Boot header | Android v4; qualified `.500` 4 KiB header retained exactly |
| GKI signature payload | Qualified 16 KiB payload retained exactly |
| AVB | SHA256_RSA4096 PASS; 100,663,296-byte `boot` partition |
| AVB public-key SHA-1 | `2597c218aae470a130f61162feaae70afd97f011` |

No replacement `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `dtbo`, or VBMeta
physical candidate was produced.

## Immutable firmware contract

| Payload | Qualified SHA-256 |
|---|---|
| Stock `system_dlkm` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| Stock `vendor_dlkm` | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| Stock `vendor_boot` | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

These partitions, VBMeta, metadata, and userdata were not written during
static qualification.  `modules.load` remains 82 entries with `wwan.ko` at
entry 28.

## Physical gate

The device was not connected when static qualification completed, so no
partition was flashed.  ENC-S01 must next pass the boot-only TWRP metadata/PIN
oracle, `/data` F2FS mount with inlinecrypt, and Android existing-user0 unlock.
The exact Phase 1 boot SHA-256
`816eafa394c5aaab26d2bff9ad37b612045b8fd3a5702f81d3f6fc81e78db35f`
remains the rollback artifact.  ENC-S02 must not be applied before this gate.

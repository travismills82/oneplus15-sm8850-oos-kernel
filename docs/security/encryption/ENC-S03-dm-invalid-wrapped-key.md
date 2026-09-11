# ENC-S03: device-mapper invalid wrapped-key handling

## Scope

ENC-S03 fixes the device-mapper pass-through path for invalid hardware-wrapped
keys. It makes `dm_derive_sw_secret()` stop after the first underlying device
instead of submitting the same failing unwrap request to every device in a
multi-target mapping. Successful key derivation, valid encrypted I/O, key
formats, DUN calculation, filesystem policy, and on-disk format are unchanged.
The kernel remains Linux 6.12.35 and Android KMI generation 5.

## Provenance

| Role | Commit | Subject |
|---|---|---|
| Upstream device-mapper fix | `d6d0e6b9d54532264761405a1ba8ea5bd293acb1` | `dm: fix excessive blk-crypto operations for invalid keys` |
| Android Common 6.12 backport | `fd32a2d0c1f31e8410f30aedbcbef63577b1ae96` | `BACKPORT: FROMGIT: dm: fix excessive blk-crypto operations for invalid keys` |
| Local provenance-preserving backport | `96fce14273ea4f73ccd453bf4ba3cb31c6e24ec9` | same subject and original Eric Biggers authorship |

The Android backport correctly targets the older derive-secret-only interface
present in this generation-5 tree. It depends on the existing wrapped-key
pass-through implementation and requires no local conflict adaptation.

## Compatibility assessment

- Valid wrapped-key behavior: unchanged.
- Invalid wrapped-key behavior: one hardware request, then the real error is
  returned.
- FBE, metadata encryption, and inlinecrypt formats: unchanged.
- F2FS, fscrypt, UFS crypto, and Qualcomm ICE interfaces: unchanged.
- ABI/KMI declarations and provider CRCs: unchanged.
- Kconfig: unchanged.
- Userspace tools impact: none.
- Reformat or key regeneration: not required.
- Stock supporting partitions: unchanged.

## Static validation

The following commands passed from `kernel_platform/` on runtime source
`96fce14273ea4f73ccd453bf4ba3cb31c6e24ec9`:

```text
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel build \
  //common:kernel_aarch64_abi \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //common:kernel_aarch64_abi_diff
tools/bazel run //common:kernel_aarch64_abi_dist -- \
  --destdir=out/enc-s03-abi-dist
tools/bazel build --kbuild_symtypes //common:kernel_aarch64
tools/bazel build //soc-repo:canoe_perf_abi
```

| Gate | Result |
|---|---|
| Canoe distribution | PASS |
| Enforced common ABI | PASS; empty diff |
| Strict KMI symbol list | PASS |
| Symtypes | PASS |
| Canoe device-module-inclusive ABI dump | PASS |
| ABI reference update | NONE |
| Configuration change | NONE |

The generated configuration is byte-identical to the physically tested Phase
1 and ENC-S01 configuration, SHA-256
`166e1db5f0a5e5e33b801413d3f5255f8f92dcf8e210317d24d27cb3389897d4`.
The generated `Module.symvers` is also byte-identical to the qualified provider
oracle: 9,356 records, 575,272 bytes, SHA-256
`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`.
This replays the accepted 1,020-module, 57,216-edge stock contract through
exact provider identity; the removed stock-binary extraction roots prevent a
fresh binary rescan in this worktree.

All 103 installed GKI modules are signed with SHA-512 by the expected build
key. There are no unsigned modules, signer mismatches, SHA-1 fallbacks, host
OpenSSL fallbacks, ABI suppressions, KMI changes, or module-security changes.

## Boot-only cumulative canary

ENC-S03 is physically tested together with the independent low-risk ENC-S02
status-code correction. Their cumulative artifact contains no other runtime
source change.

| Item | Value |
|---|---|
| Path | `out/oos1610500-ack-6.12.35-encryption-hardening-enc-s02-s03/boot.img` |
| Size | 100,663,296 bytes |
| SHA-256 | `a5bd3347805c8f6bc96217a6e36eb4f8764be8654e531e938a769a5c29ad1511` |
| Embedded Image size | 39,959,040 bytes |
| Embedded Image SHA-256 | `6b66e3833bdf4629f909f0ff22dfcc73838d859c18829f0b0b863b2979b3e95a` |
| Kernel release | `6.12.35-android16-5-o-g96fce14273ea-4k` |
| Boot header | Android v4; qualified `.500` 4 KiB header retained exactly |
| Qualified tail | Existing qualified 16 KiB tail retained exactly |
| AVB | SHA256_RSA4096 PASS; 100,663,296-byte `boot` partition |
| AVB public-key SHA-1 | `2597c218aae470a130f61162feaae70afd97f011` |

No replacement `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `dtbo`, VBMeta,
metadata, or userdata artifact was produced or written during static
validation.

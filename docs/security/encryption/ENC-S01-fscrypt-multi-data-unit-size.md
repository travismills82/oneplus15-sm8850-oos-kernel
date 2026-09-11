# ENC-S01: fscrypt multi-data-unit-size key setup

## Scope

ENC-S01 is the first Phase 2 encrypted-storage series.  It changes only the
built-in fscrypt key cache and retains Linux 6.12.35, Android KMI generation 5,
the qualified configuration, and every stock supporting partition.

## Provenance

| Role | Commit | Subject |
|---|---|---|
| Upstream Linux fix | `dd015b566d505d698386103e9c80b739c7336eb8` | `fscrypt: Fix key setup in edge case with multiple data unit sizes` |
| Android Common 6.12 backport | `6a379f037a29b228644ddcfcd7db0e99d8e24767` | `BACKPORT: fscrypt: Fix key setup in edge case with multiple data unit sizes` |
| Local provenance-preserving backport | `dbb2a6c1df7fd018aa74353762e6b4659b6a4593` | same Android subject and original Eric Biggers authorship |
| Generation-5 OOS adaptation | `ad4ab6465d9925abe369e20d001bd2c765417da1` | `ANDROID: fscrypt: preserve generation-5 master-key layout` |

The Android backport includes its authoritative hardware-wrapped-key conflict
resolution and `kzalloc()` compatibility adjustment.  The downstream commit
is separate and does not masquerade as upstream work.

## Correctness impact

Before this fix, non-file-scoped v2-policy keys were cached only by master key,
mode, and IV policy.  Two otherwise matching policies with different data-unit
sizes could reuse the first cached `blk_crypto_key`, causing data to be
encrypted or decrypted with the wrong unit size on an `inlinecrypt` mount.

The selected implementation keys the cache by HKDF context, encryption mode,
data-unit-size bits, and inline-versus-software implementation.  It preserves
hardware-wrapped-key preparation through `fscrypt_prepare_inline_crypt_key()`;
key programming and eviction still traverse the qualified fscrypt ->
blk-crypto -> dm-default-key -> UFS -> QTI ICE path.

## Generation-5 ABI adaptation

Plain application of the newer fix shrank `struct fscrypt_master_key` from 872
to 360 bytes by replacing three fixed prepared-key arrays with one list head.
That is a real change to an Android ABI-represented type, so it was not hidden
with an ABI reference update, ignore rule, type string, CRC edit, or
MODVERSIONS change.

The adaptation restores all three qualified arrays at their exact old offsets
and uses the first bytes of the now-obsolete private `mk_direct_keys` storage
as the new list-head anchor.  Compile-time size and alignment assertions cover
that storage.  The resulting ABI comparison is empty, with the qualified
872-byte size and every later member offset restored.

## Compatibility assessment

- On-disk format: unchanged.
- fscrypt policy/key descriptor format: unchanged.
- DUN format and valid-I/O behavior: unchanged.
- Userspace fscrypt/F2FS tools impact: none.
- Reformat or key regeneration: not required.
- Configuration expansion: none.
- Stock DLKM or vendor-boot change: none.

## Static validation

The exact final source is `ad4ab6465d9925abe369e20d001bd2c765417da1`.

| Gate | Result |
|---|---|
| `tools/bazel build //soc-repo:canoe_perf_dist` | PASS |
| `tools/bazel build //common:kernel_aarch64_abi //common:kernel_aarch64_abi_kmi_symbol_checks //common:kernel_aarch64_abi_diff` | PASS; empty diff |
| `tools/bazel run //common:kernel_aarch64_abi_dist -- --destdir=out/enc-s01-abi-dist` | PASS; empty diff |
| `tools/bazel build --kbuild_symtypes //common:kernel_aarch64` | PASS |
| `tools/bazel build //soc-repo:canoe_perf_abi` | PASS; device-module-inclusive ABI dump |
| Strict KMI symbol-list enforcement | PASS |

The final generated configuration is byte-identical to the Phase 1 physically
tested configuration.  Its SHA-256 is
`166e1db5f0a5e5e33b801413d3f5255f8f92dcf8e210317d24d27cb3389897d4`.
The final `Module.symvers` is also byte-identical to the Phase 1 provider
oracle: 9,356 records and SHA-256
`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`.

All 103 generated GKI modules have SHA-512 signatures from the expected build
key.  The boot-only FBE-canary artifact is:

| Item | Value |
|---|---|
| Path | `out/oos1610500-ack-6.12.35-encryption-hardening-enc-s01/boot.img` |
| Size | 100,663,296 bytes |
| SHA-256 | `a4099be9d60f2d81268d64ab9e1d38efdf3a8a6c15638ba5d60f3384f423cf3d` |
| Embedded Image SHA-256 | `e4977a2531486ee3bc7b49330c787775b9db0256b668b4fa68ca87331f02fe9e` |
| Kernel release | `6.12.35-android16-5-o-gad4ab6465d99-4k` |
| AVB/container | PASS; normalized to the qualified `.500` boot contract |

The physical TWRP/Android FBE oracle remains pending.  No later ENC series may
be stacked until this high-risk series passes that gate.

## Rollback

The rollback artifact is the exact Phase 1 physically tested boot image with
SHA-256
`816eafa394c5aaab26d2bff9ad37b612045b8fd3a5702f81d3f6fc81e78db35f`.
Only the active boot partition may be changed by the physical oracle.

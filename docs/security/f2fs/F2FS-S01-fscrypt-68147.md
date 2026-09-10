# F2FS-S01: CVE-2026-68147 fscrypt inline-crypto lifetime fix

## Provenance

- upstream: `6fe4e4b8259e1330945b5f3c9476e08473b8e0e8`
- Linux 6.12 stable: `97a688563be71ec6fefc071aff69a66c69dbe244`
- Android KABI backport: `ed9f54727ab32304aaa43ac8ad777e75ab0747d4`
- local commits: `b6baf61f012e`, `9efa70c2122c`, `594ae87e5bbe`
- CVE: `CVE-2026-68147`
- Fixes: `22e9947a4b2b` (`fscrypt: stop holding extra request_queue references`)

## Resolution

The allocation-free device enumeration is retained.  The callback that fills
the caller-provided eight-device array is stored in the first existing Android
KABI reserve word, and all access goes through a typed accessor.  This keeps
the exact `struct fscrypt_operations` size and existing member offsets while
avoiding allocation failure during key eviction under reclaim.  The pre-existing
OnePlus hardware-wrapped-key path remains intact.

## Impact

- security: prevents key-lifetime UAF when device-array allocation fails
- encryption: changes enumeration mechanics, not device selection or key data
- disk format: none
- userspace tools impact: none
- ABI/KMI: enforced common ABI diff empty; strict KMI and symtypes pass
- module contract: final `Module.symvers` is byte-identical to the qualified
  6.12.35 reference after this series
- FBE: static contract preserved; physical existing-user0/TWRP decryption is
  still required
- rollback: boot the exact qualified 6.12.35 image with SHA-256
  `bbf3e9ed0fae1e55b3c7522cadff9509decc24bfd5a412ae66d4cbebea5effdc`

The complete Canoe distribution and enforced ABI distribution targets pass on
the series.  No configuration, DLKM, vendor boot, device tree, or on-disk
format was changed.

# F2FS-S02: fscrypt and device-mapper security fixes

## Commits

| Local commit | Authoritative 6.12 commit | Purpose |
|---|---|---|
| `2895e19ff078` | `cfe27f8aff2e` | exclude problematic non-inline Crypto API engines |
| `3414c263d56f` | `deff41898a5a` | CVE-2026-68148 legacy direct-key superblock isolation |
| `589b6513594d` | `3277e56be44b` | dependency: scoped class support |
| `b7d088277040` | `3ce94fc540f0` | dependency: corrected scoped class semantics |
| `6650f661953e` | `cf71329cf971` | dependency: `kernel_cred()` helper |
| `d9c2abeec2a7` | `fd5aaed92afd` | dependency: scoped kernel credentials |
| `6ba85d21f2ba` | `d3eb8451d529` | CVE-2026-72103 DM table-device credential isolation |
| `ce53225ad0f7` | `6a67c460b123` | CVE-2026-74595 mount-idmap owner check |

The `fscrypt_private.h` conflict for `cfe27f8aff2e` was resolved by preserving
the OnePlus hardware-wrapped-key size definitions and adding the upstream
Crypto API mask alongside them.  The remaining commits applied without
semantic conflict.  The two official `scoped_class()` dependencies are kept
in their stable dependency order; the build does not use an unreviewed local
credential workaround.

## Impact

- security: closes a legacy-v1 key lifetime UAF, prevents DM from retaining a
  caller thread keyring, and checks policy ownership through the issuing mount
  idmap
- encryption: inline crypto remains preferred; the fallback remains available
  but cannot select non-inline engines that allocate memory or are kernel-only
- disk format: none
- userspace tools impact: none
- ABI/KMI: no intended exported ABI change
- configuration: unchanged
- FBE: no policy, key format, dm-default-key, UFS, ICE, or F2FS disk-format
  change
- rollback: exact qualified 6.12.35 boot image

`CVE-2026-68148` affects only legacy v1 `DIRECT_KEY` policies, but its fix is
small and prevents cross-superblock key lifetime corruption.  OxygenOS user0
normally uses fscrypt v2; that does not make the vulnerable compiled-in v1 path
safe to leave unfixed.

The complete Canoe distribution, common ABI/KMI targets, symtypes build,
Canoe ABI dump, and enforced ABI distribution passed after this coherent
series.  The resulting `Module.symvers` remained byte-identical to the
qualified 6.12.35 provider contract.

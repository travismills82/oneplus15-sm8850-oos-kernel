# SUSFS v2.4.0 on the ColorOS .102 Linux 6.12.117 boot-only kernel

The candidate was built from `d061775d9240fd28bfe3e11d3430f05400fb4413`
plus the SUSFS source patch. Source integration preserves the later README-only
commit `df5befa18c218708b5279d13a6b0a5d8aa268e40`; the kernel diff is unchanged
from the build, not rebuilt for publication. Existing release assets remain
unchanged. The physical canary covers one OnePlus 15 on the installed
CPH2749_17.0.0.102(EX01) ROM: its stock boot, DTBO, vendor_boot, vbmeta and both
DLKM hashes agreed with the established .102 boot compatibility contract.

The old SUSFS source pin was
`68f62fae75ada3959f816841a3c64299383521ed`. The replacement is the
[v2.4.0 version-bump revision](https://gitlab.com/simonpunk/susfs4ksu/-/commit/f83ae68add82d1c564e9428ba81dd537fa721924)
of the upstream `gki-android16-6.12` branch. The underlying 6.12 core
patch, shared command definitions, and userspace CLI source are identical
between those exact pins. Do not apply the entire core patch a second time.

The five source files carry the incremental rate-limited diagnostics, version
announcement, and output of the matched KernelSU compatibility patch
for policy-context parsing. To obtain that output, apply the respective
`10_enable_susfs_for_ksu.patch` to the old official KernelSU pin `28db74f6`
and the upstream-documented new pin `019a3c01`, then compare the patched
`kernel/` trees. Only three KernelSU files differ. The existing policy setup
and enforcement are unchanged: as in the upstream patched result,
`rules.c` does not register the new optional type metadata, so lookups
retain the backed-up policy fallback. Rate-limiting has an empty macro for
the logging-disabled configuration; the unused upstream error macro is omitted.

KernelSU 32716 / API 6 and its Manager signing contract remain unchanged.
No new hide or spoof settings are applied. The existing config, boot-only
provider model, module policy and security settings are preserved.

## Offline verification

- ARM64 object builds for `fs/susfs.o`, `feature/selinux_hide.o`,
  and `selinux/sepolicy.o` succeeded. The production Kleaf SUSFS object
  contains the v2.4.0 version and rate-limited log call.
- The production-generated config is byte-identical to the released .117
  config; all 1,070 original stock module input hashes match the inventory.
  Input integrity is not yet a candidate module-oracle result.
- The historical .115 SUSFS source suite passed 10/12. The two failure checks
  compare defconfig and scheduler files to .115; each affected file matches
  released .117 exactly. No assertions were weakened.
- Common build, strict KMI and ABI generation passed. The full
  `Module.symvers` and ABI dump SHA256s match the .117 release. Enforced
  GKI comparison still FAILS with the inherited differences (exit 4).
- Production stock-module oracle: 1,070 records, 1,032 compatible, 38
  dormant, 60,701 hard imports and 61,777 CRC records satisfied. No missing
  providers, mismatched CRCs, namespace failures, active blockers or
  production-provider mismatches. Boot v4 scope and offline AVB verification
  passed with the existing development boot signing contract.

## Exact physical canary

- Running kernel: `6.12.117-android16-6-o-gd061775d9240-4k`.
- Boot SHA256: `03240d37421c716d4aa32fdf32efb9f9c4e6541259112b7e9ae27450867b292a`.
- Image SHA256: `d154c5989d62932ac106d28f6d980d2494e035172a6a539dc059374a9621a694`.
- Source patch SHA256: `12cf2758f096bfca78e4f17e2b51ab1e11b1017e73fb30dd073fbeef73e08d70`.
- Fastboot confirmed unlocked, slot A, no snapshot, and 96 MiB boot capacity.
  Normal Android properties did not reflect that raw state; the live bootconfig
  also independently confirmed unlocked/ORANGE.
- Only `boot_a` was written. Full recovery read-back matched; all 13 protected
  companion hashes were unchanged, including both DLKMs and the existing
  Magisk-patched init_boot. No partitions were resized, and no key, userdata,
  slot or verification settings were changed.
- SUSFS getters returned v2.4.0, GKI and the expected feature list. The temporary
  read-only probe used the existing Magisk root grant; KernelSU Manager root
  and all optional SUSFS behaviors were not independently qualified.
- Android boot completed, user 0 reached RUNNING_UNLOCKED, F2FS used inlinecrypt,
  SELinux was enforcing, both DLKM verity targets reported `V 0`, and all 691
  loaded module build IDs matched stock. Live config SHA256 matches the build.
- The same boot stayed healthy through 243 seconds. Pstore was empty;
  the available logs contained no panic, oops, hung-task, module CRC/symbol,
  KCFI or verity-corruption signature. The hung-task timeout remains 60 seconds.
- Early traces included the familiar HFI IRQ-wake and HBP frame warnings.
  Seven duplicate proc-directory warnings came from retained OPlus modules
  registering `/proc/oplus_mem` or `/proc/task_info`; both directories remained
  present. The retained sync-fence module requested unsupported mode 0666;
  the unchanged sysfs code masks it before creation. The latter warning
  classes were not in the earlier CPH2747 .117 capture; attribution to
  firmware, load order, or root environment is not independently established.
  All originating modules loaded with their stock build IDs. No warnings
  were suppressed.
- Boot rollback: the original stock boot was saved and independently
  hash-verified before the write. SHA256:
  `03fb9cfa5edfadd5044b442d68ca906225f2733f2ce71045248843bb15943fdc`.

This is a scoped boot/FBE/SUSFS test, not full firmware or hardware
qualification, and not a release.

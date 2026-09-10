# F2FS-S06: memory-safety and corruption hardening

## Authoritative 6.12 commits

This final selected series is applied as individual Linux 6.12 stable commits:

1. `44a79437309e` — initialize every `extent_info` field before use
2. `dea243f58a83` — avoid an inode lifetime UAF in `f2fs_sync_inode_meta()`
3. `97df495d7541` — avoid a panic while evicting a corrupted inode
4. `70849d33130a` — bound multi-device path access
5. `0fe7976b6254` — keep `vm_unmap_ram()` out of IRQ-disabled completion context
6. `888aa660144b` — reject out-of-range dnode offsets
7. `6b9525596a83` — reject corrupted NIDs in the free-NID list
8. `1ff415eef513` — preserve the correct physical extent for swapfile mapping
9. `7c002e242ddd` — pass `sbi` to compressed-page array allocation helpers
10. `cc81768212cd` — fix a compressed-inode lifetime UAF
11. `56038756aae6` — fix an atomic-inode lifetime UAF

The page-array signature change is a required internal dependency of the
compressed-inode UAF repair, not an independently selected feature. The
swapfile fix is adapted semantically to retain this tree's existing
`f2fs_valid_pinned_area()` validation while moving `last_extent` to the full
migration scope and retrying mapping after successful migration.

## Contract

- security/correctness: uninitialized state, UAF, panic, OOB, corrupt-NID, and
  incorrect physical-mapping fixes
- disk-format impact: none
- userspace tools impact: none
- reformat required: no
- configuration impact: none
- ABI/KMI: must remain empty/clean after the complete series
- FBE impact: no fscrypt policy, key format, inline-crypto, ICE, or
  dm-default-key behavior change
- rollback: exact physically qualified 6.12.35 boot

The source changes remain within F2FS. They introduce no feature bit and no
checkpoint, NAT/SIT, compression, or encryption-policy format change.

The final complete Canoe distribution and exact common ABI/KMI targets passed
after this series.  Final symtypes, Canoe ABI, enforced ABI distribution,
module-provider identity, signing, and boot-container results are recorded in
`oos1610500-6.12.35-f2fs-static-validation.md`.

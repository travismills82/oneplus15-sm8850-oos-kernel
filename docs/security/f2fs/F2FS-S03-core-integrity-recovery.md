# F2FS-S03: core integrity and recovery fixes

## Authoritative 6.12 commits

Applied individually, in Android/Linux 6.12 stable order:

1. `c465f523333e` — zero the post-EOF page range
2. `334afc40c41c` — release the superblock-commit bio on error
3. `a7b7ebdd7045` — truncate the first page on truncate failure
4. `d6b19dacc094` — bound post-EOF invalidation and avoid unnecessary work
5. `40bf3676cb39` — correct multi-device physical-block mapping
6. `0e75a098b0a3` — avoid overflow before shifting a compressed-page index
7. `6c3bab5c6261` — remove a compressed-I/O lock inversion
8. `19d7ac99e101` — prevent age-extent insertion counter overflow
9. `baf461563a8d` — initialize one-time GC policy state
10. `473550e71565` — return the correct fsync-recovery result
11. `baf1a27e5664` — identify recoverable inodes during dry-run recovery

The post-EOF helper conflicts were resolved by retaining the OnePlus trace-hook
include and externally visible `f2fs_filemap_fault()` while adopting the exact
new invalidation range and locking behavior.  The compressed-page overflow fix
uses the existing folio conversion and casts `folio->index` before shifting.

## Contract

- security/correctness: data exposure, leak, overflow, deadlock, mapping, and
  crash-recovery fixes
- performance: only the required follow-up that limits the new post-EOF scan
- disk-format impact: none
- userspace tools impact: none
- reformat required: no
- configuration impact: none
- ABI/KMI: must remain empty/clean after the complete series
- physical gate: existing-user0 and TWRP metadata decryption remain required
- rollback: exact physically qualified 6.12.35 boot

No feature bit, checkpoint/NAT/SIT representation, compression format, or
fscrypt policy format is changed.

The complete Canoe distribution, common ABI/KMI targets, symtypes build, and
Canoe ABI dump passed after this series.  Generated configuration and
`Module.symvers` hashes remained identical to the preceding validated state.

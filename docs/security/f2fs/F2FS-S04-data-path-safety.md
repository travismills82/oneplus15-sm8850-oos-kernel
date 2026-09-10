# F2FS-S04: data-path lifetime and validation fixes

## Authoritative 6.12 commits

Applied individually, in Android/Linux 6.12 stable order:

1. `4ef30b9f1641` — size-aware F2FS sysfs reads and writes
2. `cf4a9e1bc812` — order checkpoint wakeup before the final writeback release
3. `962c167b0f26` — serialize atomic fsync dentry marking with checkpoint state
4. `c78206dcb912` — release the renamed inode on the error path
5. `f5154cf3ce1c` — avoid using `sbi` after compressed writeback completion
6. `7be222de96c0` — inspect the warm-node list before dropping the last page count
7. `75c180024631` — correct fiemap boundaries with an incomplete read cache
8. `ab1eaf9d5c99` — serialize extent-node count destruction and writeback
9. `745b49c493e4` — avoid putting an uninitialized sysfs kobject
10. `037a534f1860` — correct unwritten inline-inode address mapping
11. `aba4f94ac183` — validate every on-disk ACL entry size

Conflict resolutions retain this tree's folio conversions and OnePlus sysfs
extensions.  They move the same operations across the lifetime boundaries
required by the upstream fixes; they do not restore old unsafe ordering.

## Contract

- security/correctness: UAF, leak, OOB, uninitialized-object, race, and mapping
  fixes
- disk-format impact: none; malformed existing metadata is rejected safely
- userspace tools impact: none
- reformat required: no
- configuration impact: none
- ABI/KMI: must remain empty/clean after the complete series
- rollback: exact physically qualified 6.12.35 boot

The complete Canoe distribution, common ABI/KMI targets, symtypes build, and
Canoe ABI dump passed after this series.  Generated configuration and
`Module.symvers` hashes remained identical to the preceding validated state.

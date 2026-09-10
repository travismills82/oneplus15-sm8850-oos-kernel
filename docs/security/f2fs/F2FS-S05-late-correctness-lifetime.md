# F2FS-S05: late correctness, deadlock, and lifetime fixes

## Authoritative 6.12 commits

Applied individually, in Android/Linux 6.12 stable order:

1. `58a5deb220bc` — preserve `FI_NO_EXTENT` while destroying an extent node
2. `603c55e992f8` — keep atomic-write retry from zeroing original data
3. `550511a2470f` — validate orphan-inode entry count
4. `44480f7e3f83` — remove the `f2fs_balance_fs()` GC lock inversion
5. `b885c7783c19` — apply that ordering to the `gc_merge` path
6. `7dd01f7d0291` — handle corrupted xattr entries without walking out of bounds
7. `7dd611131d82` — keep the original page reference in `f2fs_merge_page_bio()`

The last change is the current Linux 6.12 stable UAF fix from upstream commit
`edf7e9040fc52c922db947f9c6c36f07377c52ea`.  The adjacent compressed-overwrite
deadloop repair `7cd460bd9e7c` was tested as an empty cherry-pick because its
functional changes are already present in the qualified OnePlus tree; it is
recorded as `ALREADY_PRESENT`, not duplicated.

## Contract

- security/correctness: data-loss, corrupted-metadata, deadlock, bounds, and
  page-lifetime fixes
- disk-format impact: none
- userspace tools impact: none
- reformat required: no
- configuration impact: none
- ABI/KMI: must remain empty/clean after the complete series
- rollback: exact physically qualified 6.12.35 boot

The complete Canoe distribution, common ABI/KMI targets, symtypes build, and
Canoe ABI dump passed after this series.  Generated configuration and
`Module.symvers` hashes remained identical to the preceding validated state.

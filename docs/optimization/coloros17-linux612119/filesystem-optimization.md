# Filesystem, storage and memory efficiency

- F2FS, ext4, EROFS, OverlayFS, FUSE and exFAT are built in. NTFS3 is not
  enabled; no experimental filesystem or disk format change. Android
  `/data` actually uses F2FS with inlinecrypt. Existing filesystem barriers,
  fsync and FBE/KeyMint behavior are outside the experiment.
- The v6.12.119 stable history already contains F2FS GC fixes such as
  avoiding reads of already updated pages, narrowing `gc_lock` coverage and
  correcting pinned-state/free-segment handling. Do not apply these twice
  or infer that a public upstream commit ID alone proves absence locally.
- The optional second image adds the block plug compiler-ordering fix.
  The baseline stores zero to `current->plug` at `blk_finish_plug+0x68`
  and clears `PF_BLOCK_TS` only at `+0x70`. The barrier must make the
  compiled flag-clear store precede the pointer-clear store. It is not a
  hardware fence and does not change writeback, barrier, or I/O scheduling
  behavior. B2's disassembly verified the flag store at `+0x70` followed
  by the pointer clear at `+0x74`; the scratch filesystem tests passed.
- First experiment changes only page-cache shadow shrinker budgeting when
  kernel memory accounting is disabled. It does not retain active file
  pages by fiat, disable reclaim, change discard, delay fsync, or touch the
  on-disk formats. More retained refault metadata can itself cost memory;
  measure PSI/refaults/LMKD and file I/O together under a matched workload.
- UFS core and crypto are compiled in; Canoe host, inline-encryption vendor
  backend and PHY modules remain the exact stock stack. The generic SCSI
  host PM telemetry did not disclose real UFS residency, but the actual
  platform controller exposed runtime-suspended time. The upstream
  write-pipelining toggle is left out; test FBE, persistence, writes,
  fsync latency, thermals and bandwidth before considering it independently.
- Active MGLRU mask observed `0x0003`; ZRAM live algorithm `lz4` and no
  ZRAM backing I/O in the first snapshot. `mm_stat` reported about 2.70 GB
  uncompressed, 943 MB compressed, 982 MB allocator footprint. This is a
  stock module path, with OPlus integration and possible QPACE hardware
  behavior, not evidence to tune the common kernel's compression defaults.

## Codec screening

`tools/testing/bench-coloros17-zram-codecs.c` compares 4 KiB page
round-trips for liblz4 and libzstd level 1 on a supplied test corpus.
The baseline kernel Image, a reproducible mixed code/data proxy, was
used for a host-only screen. Zstd compressed that proxy smaller
(22,427,398 vs 27,843,969 capped bytes for 10,363 nonzero pages),
at higher compression and decompression CPU time in that run.
The result is intentionally **not** an Android memory corpus, algorithm
recommendation, or ZRAM/QPACE/energy result. No realistic device workload
can be replayed against two algorithms without separately authorized
configuration and device control. Keep LZ4 and existing Android LMKD,
compaction, reclaim and swap behavior until warranted.

## Further measurements

Use matched app-launch / multitasking loops and sustained cache-pressure
intervals with identical app state and page-cache warmup. Count
`workingset_nodereclaim`, `workingset_refault_file`, PSI and app cold-start
latency, both ZRAM traffic and actual storage I/O separately, and fuel-gauge
delta. Preserve verified photos/media and never run a destructive filesystem
test on phone userdata. The ext4/F2FS scratch checks use disposable ARM64
guests, not the actual device.

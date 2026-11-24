# Filesystem, memory, ZRAM and storage

- `/data` is still encrypted F2FS with `inlinecrypt`; EROFS, EXT4,
  FUSE, exFAT and OverlayFS remain built as in B2. Keep filesystem
  barriers/durability, FBE state, encryption and stock firmware
  metadata. The live F2FS lifetime counter is about 11.6 GB decimal of
  filesystem writes; the sampled `gc_reclaimed_segments` is 1.
  Neither establishes overnight write amplification or device wear.
- The UFS host was runtime-suspended, usage 0, with reported cumulative
  active/suspended accounting of 8.15/23.29 million ms. Clock accounting
  excludes/folds some system sleep, so do not divide these by boottime.
  No retained UFS reset, F2FS integrity, inline crypto or verity failure
  was identified. Preserve filesystem writeback, discard and queue policy.
- Active MGLRU value `0x0003`; PSI memory and I/O avg10/60/300 were
  zero at the sample, with nonzero lifetime stalls. Available memory
  ~5.6 GiB. Lifetime counts: 4.10 M swap-ins, 6.76 M swap-outs, 3.93 M
  file refaults, 674,488 kswapd scans, 493,170 steals, 1,284 normal-zone
  allocation stalls and zero compaction stalls. Split a controlled
  memory-pressure workload from nighttime idle before inferring costs.
- Stock ZRAM selects **LZ4** at runtime, despite the generic config
  default for other boots. It held 3.87 GB original bytes in 1.37 GB
  allocator bytes (~2.82x effective compression); swap is 12 GiB
  nominal. The backing device statistics were all zero; writeback
  is compiled but not demonstrated active. An ACK writeback fix in an
  undeployed modular source does not change the running stock module.
- The already installed B2 shadow-node accounting fix remains sensible
  under `nokmem`. Workingset node reclaim is zero in this snapshot; we
  cannot infer a battery win from that counter without the old binary
  and the same workload. Preserve zram codec, swappiness, LMKD, and
  compaction defaults. Synthetic codec throughput/ratio tests do not
  determine ARM64 power or Android application behavior.

Run fixed read, write+fsync, app-relaunch and container workloads on both
binaries; compare delta refaults/PSI, block sectors, thermal and wall time.
No device writes or synthetic IO benchmarks were performed during this
read-only evidence collection.

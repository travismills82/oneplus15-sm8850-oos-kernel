# ColorOS 17 .115 no-GEAS post-benchmark log review

## Result

**No GPU failure found in available logs and persistent fault counters.**
Heavy GPU benchmark exercise is owner-reported and supported by benchmark app
lifecycle and GPU busy-accounting evidence. This is not a score comparison,
an isolated frame-pacing/power measurement, or full stable-release qualification.

The exact candidate remains installed on slot B:
`6.12.115-android16-6-o-ga3b501fee3c3-4k`, ColorOS
`CPH2745_17.0.0.102(EX01)`. GEAS remains deliberately excluded. No reboot, flash,
rollback, slot switch, kernel/configuration change, counter reset, thermal
tuning, push or publication was performed during this review.

This is an addendum to the immutable evidence for the
[initial physical canary](coloros1700102-linux115-no-geas-physical-20261101.md).
That report and its hash-sealed evidence were not edited.

Private evidence directory:
`/home/travis/Android/kernel-test-backups/coloros17-linux115-no-geas-post-benchmarks-20261101-5v6Bvs/`.
It contains full kernel/logcat snapshots, source-grounded counter interpretation,
GPU/thermal/service/module state, crash-record listings, capture receipts and
`analysis.json`. Raw logs contain device data and are not publication assets.

## Observed checks

| Check | Result and scope |
|---|---|
| Android/user0 | boot_completed=1, RUNNING_UNLOCKED |
| Slot/kernel | B, exact frozen .115 release; uptime consistent with the existing canary boot |
| Vendor dm-verity | V/0, no observed verification error |
| Installed boot/vendor_dlkm/vbmeta | Full device SHA256s still match frozen images |
| Magisk init_boot B | Exact unchanged qualified SHA256 |
| Updated GPU module | Actual file hash `d8711d3bb64a936e89c7df13996af3d7e4c6b71c83e217ea09e23d3abc965aa8` |
| GEAS CPU/system/data | Absent; 688 modules loaded, matching the initial no-GEAS canary |
| GPU faults | All eight exported persistent fault counters zero |
| GPU faulting processes | Empty |
| GPU IOMMU page faults | All seven exported count columns zero; process list empty |
| Retained kernel panic/Oops/KCFI/RCU/hung-task/module-version/GPU fault traces | No matches |
| Retained Android GPU-device-lost/crash/ANR evidence | No new benchmark/GPU failure record found |
| Known init child aborts | Same previously classified UFS diagnostic and qti-testscripts children, not PID1 or user0 failures |
| Pstore | No fatal record present |
| Tombstone/dropbox evidence | Tombstones listed are older Oct 26 records; no SYSTEM_TOMBSTONE dropbox entry |
| Compositor | Still hardware Qualcomm Adreno 840, OpenGL ES 3.2 |
| GEAS userspace errors | No matching error/restart record in retained logcat after excluding our diagnostic command strings |
| Temperature after load | Direct KGSL read 34.5 C; battery about 33.4–34 C; not peak benchmark temperatures |

Retained Android events identify `com.futuremark.dmandroid.application` (3DMark)
and `com.primatelabs.parkdale` (Geekbench). Geekbench's
`BenchmarkDocumentActivity` appears in the lifecycle records. Both apps were
subsequently removed; associated force-stop/process-death records have explicit
package-uninstall/state-change reasons and are not classified as GPU crashes.
No benchmark score, exact run mode/API, stability percentage or frame-time data
was supplied or recovered. The owner report must not be expanded into a claim
that every benchmark or every graphics API has passed.

## Important counter interpretation

`reset_count` read 1087 and then 1088. This does **not** establish 1088 GPU
faults/recoveries. The actual compiled driver increments `device->reset_counter`
on normal GPU boot/power-up as well as some recovery paths. For example,
`gen8_hwsched_boot()` calls `gen8_hwsched_gpu_boot()`, which increments the
counter on ordinary wake/power initialization. The counter cannot distinguish
those cases on its own; it was not reset for this review.

The independent fault and page-fault statistics are stronger evidence here:
`gpufaults` reads the eight `adreno_dev->fault_counts` entries under a read lock;
`gpufault_procs` and `pagefaults` likewise report accumulated data without
clearing it. They remain zero/empty. Fault handling in `adreno_hwsched.c` calls
`adreno_gpufault_stats()`. Source hashes used to interpret these fields are in
`analysis.json`. This is not a guarantee against every possible uninstrumented
hardware failure, but no recorded fault is being dismissed as a normal reset.

## Thermal-cap oscillation and log retention concern

The current kernel buffer contains **8,312**
`kgsl_cooling_set_cur_state` informational messages between uptimes
1140.319117 and 1152.082929 seconds: approximately 11.76 seconds, with up to
777 entries in one uptime-second bucket.

The requested caps include 726, 826, 902, 1000, 1050, 1100 and 1200 MHz;
the dominant entries are 1050/1100 MHz, with additional rapid 902/1000 MHz
switching. These messages come from `pr_info()` in the real cooling callback,
not WARN/Oops traces. The function updates the cooling state and defers
application through `kthread_queue_work()`. Therefore message count is not
proof of an equal number of actual hardware clock changes; queued work can
coalesce. No clock, voltage, governor or thermal limit was changed by this audit.

The volume has overwritten earlier kernel history. The captured kernel log
retains only uptimes **1140.319117–1456.812583** seconds. Earlier saved evidence
covers initial boot, but there is no continuous kernel log for the entire
benchmark interval. Zero faults in persistent counters support the result but
do not recreate the missing trace. Logcat retains a wider portion of app
lifecycle history.

The cooling requests show that thermal control was active under load; their
rate, logging overhead, effect on frame pacing and difference from the
GEAS-enabled baseline remain **unresolved**. Do not dismiss the oscillation as
harmless or blame GEAS removal without a matched comparison. At the later
counter capture thermal_pwrlevel and throttling were both zero. Post-load HAL
current readings and direct KGSL temperature are distinct from older cached
HAL temperatures; none supplies a reliable benchmark peak-temperature history.

The next useful diagnostic is a repeatable workload with host-side continuous
kernel/logcat capture started before the run, time-aligned temperature/cooling
sampling and scores/frame-time measurements. A matched .114 GEAS-enabled
comparison would require separately authorized image changes; none was done
here. No suppression or thermal-policy fix has been applied merely to quiet logs.

## Qualification boundary

This adds positive post-load GPU evidence to the canary. It does not establish
performance equivalence, gaming frame pacing, sustained thermal/power behavior,
USB-disconnected suspend, or all peripheral operation. The earlier six
startup WARNs remain disclosed; their absence from an overwritten kernel buffer
does not mean they were repaired. Enforced GKI remains FAIL with the unchanged
38 reviewed differences plus six documented .115 representation differences.

Installed image hashes remain:

- boot: `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9`
- vendor_dlkm: `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804`
- vbmeta: `54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c`
- init_boot B, unchanged: `d0fce3c1448150276e06bfca3d617605ba4f5a421d8122d18f90105b8fc2257b`

The candidate remains running. Permanent GEAS removal and stable-release
promotion are not authorized or declared by this log review.

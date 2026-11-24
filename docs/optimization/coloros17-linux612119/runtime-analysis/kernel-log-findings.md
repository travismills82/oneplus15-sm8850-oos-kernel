# Retained logs and fault triage

The handset had over 20 hours of uptime. The initial 18,890-line kernel
buffer only retained printk-clock 31704.806–31736.213 (31.4 seconds).
Printk time and boottime are not interchangeable through suspend. The
Android logcat dump preserves the available buffers, approximately
09:44–12:06 for credible wall-clock records with an older invalid-clock
crash entry; it is not the full overnight log. Earlier B2 boot and
stock-compatible captures are kept independently.

| Evidence | Classification and action |
| --- | --- |
| 100 `ktime_get+0xdc/0x118` WARN lines in roughly 8.6 printk seconds | Same `oplus_powermodel:powermodel_cluster_suspend_handler+0x58` callback, including multiple CPUs in an s2idle attempt. It calls into timekeeping while suspended. Predates B2; no new time drift or failed resume evidence. Do not change Linux timekeeping or hide it. |
| 2,205 `tmf8806` I²C `-107` log lines within 0.221 printk seconds | Same burst in the prior .119 capture. The live bus device is bound to stock `camera_extension`. These are log messages, not necessarily independent transfer failures or separate wakeups. Needs matched camera source/power sequencing and paired capture to diagnose; leave camera timing and diagnostics intact. |
| 27 touch HBP frame-read errors; charger gauge unsupported-event messages; NFC standby write errors | Stock vendor paths. Touch reads already failed in older samples. Keep distinct from the earlier HBP FORTIFY boot WARN. A power or functional regression is not demonstrated. |
| Known BHB and HBP FORTIFY warnings in the saved B2 boot log | Previously disclosed. Do not change the BPF policy, driver binary, FORTIFY, or reporting. An isolated prior SYNX cancellation error was not seen in this buffer. |
| Fatal scan across saved B2 boot and current retained window | No matched panic, Oops, BUG, KCFI/RCU failure, hard/soft lockup, hung-task report, module CRC/symbol failure, verity failure, F2FS corruption, or GPU/UFS fault/reset error in the current window. A `panic_on_rcu_stall` command-line print is not a panic. |
| Android logcat fatal records | An invalid-clock userspace SIGABRT record and one Java main-thread exception are present; neither is established as a kernel failure. Raw details stay local. |

No clock vote, regulator, interconnect, runtime-PM, filesystem or modem
failure in this window was shown to be a repeatable boot-only defect.
High message volume materially limits diagnosis: collect several bounded
late-suspend snapshots alongside passive BatteryStats for the next study
instead of inferring the missing hours from this ring or suppressing WARNs.

The upstream-vendor-style TMF helper deliberately emits two messages on
one failed read (negative status and unexpected transfer length). The
exact stock `camera_extension` binary also confirms that
`i2c_write_mask` can continue to the register write after a failed read.
That is a real driver-level read/modify/write issue: a correct repair
returns the read error without composing or issuing a write. It is not
proven to be the origin of the observed read burst. The local disassembly
is kept separately; the appropriate fix requires validating an actual
matched camera driver and HAL. Do not modify generic I²C behavior or
replace the .201 camera module merely to silence the observed diagnostics.

# Power-management audit and read-only baseline

The phone was observed over wireless ADB, not modified. The
[`coloros17-power-sample.py`](../../../tools/testing/coloros17-power-sample.py)
tool reads the active boot image checksum, boot ID, elapsed time
and available counters. It never enables a tracepoint, resets counters,
changes settings or runs a continuous polling loop. Derived intervals
reject cross-reboot and cross-image comparisons.

| Domain | Baseline evidence | Assessment |
| --- | --- | --- |
| Suspend | `[s2idle] deep` (s2idle selected); PM async enabled. An early snapshot had 732 successes / 459 failures since boot; a later sample had 952 / 596. Resume failure counters were zero in the sampled records. | These are boot-lifetime totals, not failure rates for identical work. Freeze aborts and `alarmtimer.2.auto`/`-EBUSY` coexist with successful sleeps. Do not force deep or disable alarms. |
| CPU | Six CPUs in policy0, two in policy6, SCMI + stock WALT; hardware min/max 384–3,628.8 MHz and 768–4,608 MHz. One sampled effective constraint was 787.2–1,996.8 and 883.2–2,438.4 MHz. | Effective min/max are dynamic. No trustworthy causal breakdown of Power HAL, thermal and user constraints. Zero observed WALT frequency rate-limit values do not justify imposing arbitrary rate limits. |
| Idle | CPUIdle usage/residency from each CPU, including WFI and cluster-labeled idle states; per-policy residency and IRQ/softirq/schedstat available. | Individual CPU residency is not SoC sleep time. The samples do not attribute migrations to a specific caller or prove foreground latency. |
| Wakeups | In a 129.69-second awake interval, `qcom_rx_wakelock` had 12,948 events and about 129.77 additional seconds of cumulative active time. Stock UART, BT, touch and IPA/RMNET sources also advanced. | Strong candidate for *investigation*, not estimated energy or cause of screen-off drain. Wireless ADB and ordinary foreground traffic can materially bias the interval and accounting endpoints. Never disable the wake lock. |
| Qualcomm sleep stats | Stock qcom sleep/subsystem modules present. Read-only PowerStats inventory reports APSS, MPSS, ADSP, SoC and low-power states plus CPU/GPU energy channels. The text dump exposed historical logger metadata, not attributable current-domain deltas. Debugfs sleep files unavailable. | No measured interval subsystem residency or rail energy. No activation of vendor debug controls, ioctl polling, Perfetto or tracefs. |
| GPU | Stock KGSL/GEAS and the Adreno devfreq device, GPU clocks counters and IFPC counter visible; runtime status active in the awake sample. | Cumulative driver counters are not calibrated energy. Avoid reading potentially resetting `gpubusy` counters; do not alter devfreq or GEAS. |
| Storage / memory | Disk I/O counters, LRU generations, PSI and ZRAM usage sampled. Generic SCSI-host PM reported `unsupported`; the actual platform UFS controller reported ~1.45M ms active and ~4.31M ms runtime-suspended cumulatively. Its `autosuspend_delay_ms` read returned EIO. | Real UFS runtime suspend works despite limited telemetry. No change to PM enablement, clock policy or autosuspend delay. |
| Thermals / charging | No power rails attached at both sample endpoints, 26.6 °C at the first endpoint, framework thermal status 0. Gauge supplied charge and voltage. | Approximate mAh from the gauge only. mWh computed from endpoint voltage, not measured by an inline meter or energy rail. BatteryStats estimates are not energy measurements. |
| Workqueues / timers | Final config selects `CONFIG_WQ_POWER_EFFICIENT_DEFAULT=y`, `CONFIG_NO_HZ_IDLE=y`, HZ=250; trace statistics unavailable. | No global timer changes, workqueue reassignment or forced timer coalescing. APIs used by stock modules must retain their semantics. |

For an attributable idle comparison, sample endpoints only over the same
network conditions and with the same user configuration, without keeping
the phone awake over ADB. Collect controlled Wi-Fi and cellular intervals
separately. A diagnostic interval with extra tracing needs its own control
because tracing changes power, scheduling and wakeup activity.

The existing OPlus power-model warning is a stock vendor callback reading
monotonic time during the suspended timekeeper window. No change to generic
timekeeping semantics or warning generation. Preserve Bluetooth, alarms,
emergency calling, eBPF networking, AVB and thermal protections.

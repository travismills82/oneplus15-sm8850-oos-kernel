# Wakeups, suspend and workqueue evidence

- At 20 h 43 min uptime: 15,685 successful suspend attempts, 7,794
  failures/aborts, zero failures in any resume stage. The kernel counted
  6,316 failed freezes, 1,311 suspend-stage failures, 98 late and 50 noirq
  failures; the subtype counters are not a complete partition of the total.
  Last failure was `freeze`, errno `-16`; the retained last-device field
  `alarmtimer.2.auto` is not necessarily from that same attempt.
- Retained logs show repeated pending `timerfd` and occasional QRTR and
  FastRPC wakeups, early-wake aborts and `wq_busy=0` during reported
  freezer aborts. The later last resume reason was an early WLAN wake.
  These are not a demonstrated late/early device-resume defect or evidence
  for blocking Android alarms/radios. Counts are since boot and cannot
  resolve durations of individual suspend events.
- Higher sysfs wake counts included one timerfd source (15,588), QRTR
  service (4,908), QCOM RX (3,069), FastRPC (594), alarmtimer (297),
  gauge (286), userspace-abort (251), and WLAN WoW (108). These are
  source counters, not unique wake cycles; several sources can overlap.
  Individual timerfd instances need trace/process attribution before tuning.
- Lifetime IRQ activity was led by arch timer (30.5 M), apps RSC (4.65 M),
  HFI (2.24 M), WLAN CE7 (1.84 M), and display (1.58 M). These counts
  include interactive time, not a ranking of overnight energy.
- PSCI / qcom-cpu-lpm counters register the two per-CPU idle states.
  Three OEM overnight telemetry snapshots report APSS 96–99, MPSS 97–98,
  ADSP 97–99, CDSP 99–100 and vmin 92–97. These are OEM state summaries,
  not a reconstructed energy balance or exact cluster collapse time.
  PowerStats residency records are retained (e.g. APSS 45,123,088 ms,
  MPSS 73,112,164 ms in the associated boot's records). The exported
  last-entry timestamps were identical to the boot epoch, so these
  cumulative values cannot be used to align individual transitions
  or attribute exact overnight residency to a power domain.
  Direct RPMh/AOSS debugfs data and per-workqueue historical timers
  were not exposed.

Power-efficient workqueues were already enabled live. Do not change
workqueue placement, kill sessions, globally coalesce timers, block
notifications, or modify modem or wake alarms based on cumulative counts.
The upstream short-circuit optimizations reduce extra work during each
attempt, not the number of legitimate wakeups or retry attempts.

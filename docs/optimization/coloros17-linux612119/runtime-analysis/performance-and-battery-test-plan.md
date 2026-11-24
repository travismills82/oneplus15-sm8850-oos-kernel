# Physical A/B test plan (not authorized for execution)

A is the currently installed, hash-verified B2 (.119). Compare it
against the separately packaged freezer-only B3 first. To isolate
workqueue behavior, then compare B3 with B4, which adds only that patch;
check each manifest and never rename a candidate as B2. Keep the same
device, firmware, slot, init_boot,
recovery and stock DLKM. No resets or "force idle" on the physical device.
Fresh preflight must verify battery, firmware, snapshot state, Fastboot
identity, full partition size, recovery availability and the exact input
bits before any later authorized one-partition flash.

1. Run at least three unplugged overnight screen-off trials per build
   (balanced A/B order). Match starting charge band, ambient temperature,
   radios/VPN, carrier signal, installed apps, AOD and alarm schedule.
   Use wireless ADB at the boundaries only; do not poll the handset or
   stream logs throughout an idle interval. Let it settle after flash
   and after each battery measurement. Preserve battery history and
   exact start/end counter, suspend/wakeup/IRQ deltas, thermal and device
   residency evidence. Reject charging, clock jumps and unplanned
   foreground activity; document legitimate wakeups instead of ignoring them.
2. Use timed local video, scripted Wi-Fi and cellular browsing, audio,
   app launch/scroll, a fixed gaming replay, and grouped mixed usage.
   Match refresh/brightness, rendering settings, content, cache policy
   and application versions. Compare mAh/hour and energy per completed
   workload, dropped frames, median and p95 frame/launch latency,
   temperature, and state deltas. Rail estimates need non-flat verified
   counters; use an external meter for precision if available.
3. Exercise F2FS fsync/remount and inode/metadata stress only on
   disposable fixtures. Observe normal workload PSI, refaults, UFS
   runtime status and zram read/write counters. Check real Droidspaces
   start/stop/restart and namespace cleanup; confirm Wi-Fi, voice,
   LTE, Bluetooth, VPN, tethering, camera, graphics, OTG and sensor
   behavior without changing module providers or security settings.
4. In a separate matched experiment, collect a bounded suspend trace
   around a fixed repeatable idle series. Measure freezer/thaw time,
   suspend attempt/success/resume deltas and alarm/IRQ causes. Trace
   overhead invalidates the idle battery trial; record it separately.
   Do not attribute every abort to freezer performance.

Useful endpoint and historical commands (captured artifacts stay private):

```sh
python3 -B tools/testing/coloros17-power-sample.py capture \
    --serial "$ADB_TRANSPORT" --boot-sha256 "$EXPECTED" --output "$PRIVATE/start.json"
python3 -B tools/testing/coloros17-power-sample.py interval "$PRIVATE/start.json" "$PRIVATE/end.json"
python3 -B tools/testing/coloros17-runtime-history.py "$PRIVATE/history.txt" --min-seconds 300
python3 -B tools/testing/coloros17-powerstats-meter.py "$PRIVATE/meter.pb" \
    --start "$ISO8601_WITH_OFFSET" --end "$ISO8601_END_WITH_OFFSET"
```

Compare the variation across repeated trials, not just the best sample.
Retain the patch only if health is unchanged and the targeted efficiency
improves or the measured result justifies the added code. A VM suspend
freezer test is not real SoC suspend or battery data.

## Rollback contract

Before any future approved flash, take a fresh full `boot_a` read-back
and require SHA256
`29002f26c6b5bb12dd59dd1887b4b24dbcf31297bd2118277eaed93dfd90946e`
for the current B2. Keep the frozen B2 image and earlier `aabd0c59d...`
rollback separately available on the host. On a failed candidate and
authorized rollback, restore only the verified original image to
`boot_a` using the established unlocked Fastboot/recovery route. Verify
the complete read-back, slot A, no changed companion hashes, and
Android/user0/F2FS inlinecrypt. Do not switch slots, wipe userdata,
change verification settings or update the vendor stack.

# Controlled A/B test and exact rollback

**Not authorized to install or change the handset in this investigation.**
Keep A (the currently installed .119) until a separate physical test is
authorized. B is a separate boot-only artifact only if the offline gates pass.
Use the [reclaim-only B1](qualification-results.md) for the first A/B pairs
to isolate the efficiency change. Run B2 separately to qualify the
additional block-ordering stability fix.

## A/B protocol

1. Use the same CPH2747/ColorOS `CPH2747_17.0.1.201(EX01)`, active slot A,
   exact stock DLKM, companion partitions, root policy and apps. Hash-check
   the actual boot partition and record the source/payload per run. Do not
   update firmware between runs. Confirm idle OTA/snapshot and recovery
   access separately before any approved flash.
2. Repeat randomized AB/BA pairs (at least three usable pairs per main
   workload); let the phone cool and stabilize after boot. Keep starting
   state-of-charge range, ambient temperature, apps, SIM, RF conditions,
   brightness/refresh, notifications, network type and workload constant.
   Reject intervals with charging, interrupted run, changes in signal or
   app workload, battery-counter reset, or device reboot.
3. Capture read-only endpoints rather than keeping wireless ADB active
   throughout standby. No battery statistics resets or forced power policy.
   Use/replicate the same endpoint overhead for A and B. Endpoints alone
   cannot establish continuous screen-off time; record it independently.
4. Compare median paired mAh/hour and percent-points/hour, with the
   per-pair range and within-A variance. Estimate mWh only when documented
   gauge units/voltage are valid, or use a calibrated meter/rail instrument
   that does not charge the battery or inhibit suspend. Treat quantization
   and thermal state as measurement uncertainty, not savings. No promotion
   if savings do not exceed ordinary repeat-run variation or cause a
   reproducible functional, latency, power or thermal regression.

| Workload | Required observations |
| --- | --- |
| Overnight screen-off and daytime standby | Energy delta, exact duration, suspend successes and categorized aborts, sleep residency/wakeups, radio and push notification health. |
| Wi-Fi and cellular browsing, reconnect, VPN/tether | Same scripted navigation/media and RF/signal; traffic and CPU cost; reconnect, calling and accounting. |
| Launch, scrolling, app cycling under memory pressure | p50/p95 cold/warm launch, frame timing, PSI, reclaim, page-cache refaults, ZRAM traffic and LMKD kills. |
| Video, gaming, audio | Match settings, media, scenes and refresh; GPU frequencies/residency/faults, temperature, throughput and frame pacing; preserve camera, HDR and display sync. |
| Droidspaces | Same trusted ARM64 rootfs and runtime version; repeat image mount, namespace/IPC/cgroup workload, filesystem and networking, exit cleanup; compare host idle after stop. |
| USB/OTG, suspend/resume, mixed day | Read/write/eject on a spare disk, not destructive storage tests; confirm alarms, pin/fingerprint unlock, FBE, inlinecrypt, deep-sleep observation and warning/error deltas. |

## Collection

```sh
python3 tools/testing/coloros17-power-sample.py capture --serial '<ADB serial>' \
  --boot-sha256 '<expected active boot SHA256>' --output /safe/host/start.json
# Run the timed workload without periodic ADB polling.
python3 tools/testing/coloros17-power-sample.py capture --serial '<ADB serial>' \
  --boot-sha256 '<same SHA256>' --output /safe/host/end.json
python3 tools/testing/coloros17-power-sample.py interval /safe/host/start.json /safe/host/end.json
```

## Rollback A (after a separately authorized physical test)

The exact A image is `out/coloros17001201-linux119-preflight/candidate-97d4684f2e09/boot.img`,
100,663,296 bytes, SHA256
`aabd0c59d8cb6c9b27448c4c947fda90471e1fbb194837803f2e5b1b824448e4`.
Make and verify a fresh **full** slot-A read-back on the host before any B
write, and preserve the separately known-good .118 rollback.

If B breaks boot, FBE, firmware hardware, or power, restore only this
verified A image to `boot_a` through bootloader Fastboot on the exact
unlocked Canoe device, after verifying slot A, 0x6000000 capacity, and no
snapshot. Then reboot normally, compare full boot hash, Android/user0
unlock, F2FS inlinecrypt and both stock DLKM verity roots. Do not switch
slots as rollback; never flash init_boot, firmware, DLKM, or VBMeta or wipe
userdata. Stop if device or transport is ambiguous.

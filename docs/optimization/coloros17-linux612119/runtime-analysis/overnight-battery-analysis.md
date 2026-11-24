# Historical battery observation on B2

BatteryStats reports 18 h 01 min unplugged realtime since its current reset
anchor: 13 h 35 min screen off and 4 h 26 min screen on. Framework discharge
counters report 608 mAh screen-off and 1,254 mAh screen-on. Their respective
whole-history averages are approximately 44.8 and 282.7 mAh/h. These include
routine activity, not standardized workloads. Android's estimated component
and application attribution is **not measured rail energy**. In that
framework history, screen-off elapsed time minus uptime is about 10 h 58 min
(81% of screen-off realtime), compatible with substantial suspend;
it does not identify which hardware power domains collapsed.

## Charge-counter intervals

The local history analyzer rejects charging, display transitions, resets,
clock-anchor crossings and counter increases. It uses only observed
historical samples, with no interpolated endpoints. AOSP's
[HistoryPrinter](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/core/java/android/os/BatteryStats.java)
prints integer mAh derived from the charge counter. Nominal endpoint voltage
gives a gauge energy estimate, not a meter reading.

| Local time, November 24 | Display | Gauge decrease | Rate | Estimated energy |
| --- | --- | ---: | ---: | ---: |
| 00:08:58–03:38:59 | Off | 56 mAh | 16.0 mAh/h | 200 mWh |
| 03:45:21–06:43:53 | Off | 60 mAh | 20.2 mAh/h | 208 mWh |
| 06:55:54–08:28:20 | Off | 44 mAh | 28.6 mAh/h | 156 mWh |
| 09:04:07–09:46:20 | On | 224 mAh | 318.4 mAh/h | 777 mWh |

The first two screen-off observations total 116 mAh over 6.48 h
(17.9 mAh/h) for those two intervals only. A brief screen-on interval and
unsampled boundaries separate them. Doze can occur with the display off.
The observation is neither total whole-night drain nor A/B evidence.

The [AOSP PowerStats](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/services/core/java/com/android/server/powerstats/PowerStatsService.java)
meter, model and residency history is saved locally. Seven declared meter
channels produced flat energy values across 19 samples in each of the two
inspected off-screen windows. Treat these rails as uninformative for energy
attribution, not as a measured zero-watt CPU/GPU. Use a repeated
charge-counter delta or external meter for A/B; do not attribute discharge
to individual apps, modem, Bluetooth or Wi-Fi from a model alone.

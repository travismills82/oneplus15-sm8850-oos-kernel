# CPU, scheduling, GPU and display

The handset runs SCMI with WALT on both clusters. PELT remains compiled
and part of the Android core scheduler, but its presence does not prove
it drives the vendor governor. The live proxy-execution switch read
`0` despite `CONFIG_SCHED_PROXY_EXEC=y`. Do not import the ACK proxy
donor/mutex series as a WALT optimization or activate it to test power.
Keep OPlus task classification, input/application boosting, cpusets,
uclamp and the Energy Model intact.

Policy 0 residency had 1,053,938 of 3,177,977 units at its lowest
reported OPP (33.2%) and 1,203 at its highest (0.038%). Policy 6 had
2,802,487 of 3,177,979 at its lowest (88.2%) and 221 at its highest
(0.007%). This is aggregate frequency residency, not idle energy.
Observed effective policy limits differ from the available table; no
owner-specific QoS trace shows a floor sticking after a workload. PSCI
and qcom-cpu-lpm report cluster-state accumulation on all CPUs. The
captured CPU pressure (6.07% some over five minutes) is mixed-use data.
An IRQ histogram without a paired screen-off interval cannot attribute
unnecessary migrations, boosts or scheduling latency.

The confirmed stock `msm_kgsl` and GEAS modules are the live providers.
Adreno840v2 was at 222 MHz at one endpoint. Its IOMMU devices were
runtime-suspended while the GPU platform parent's runtime status was
active. These describe different devices/states; the software status
does not prove GMU/IFPC residency or a leaked GPU clock vote. Cumulative
clock buckets and IFPC count were captured, with no supported GPU
devfreq residency/bus-vote trace. The stock driver's `reset_count`
is not a fault count: related source increments it after successful
GPU start as well. Neither GPU faults nor reset failures matched the
retained kernel window. The transient `gpubusy` sample cannot
establish lifetime GPU busy fraction (see collection caveat).

The SurfaceFlinger TimeStats snapshot is not a matched frame-pacing
benchmark. Display records show 60/120 Hz transitions, not evidence
of a stuck refresh rate. The battery read 27.0 °C and Android thermal
status was 0 at the sample; this is not an overnight thermal log.
Leave WALT parameters, GPU voltage/frequency,
GEAS, bandwidth votes, thermal policy and display governor unchanged.
Future idle-to-render tests should correlate cumulative GPU metrics with
frame timelines and a non-flat energy meter. Replacing proprietary
GPU/display providers remains a matched DLKM problem, not a boot-only
efficiency patch.

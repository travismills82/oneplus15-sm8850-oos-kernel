# Candidate B08 SFQ coherence audit

## Result

SFQ is compiled and attached, but the qualified B07 runtime evidence classifies
the affected instances as **ACTIVE BUT IDLE**. It is not selected for B08.

`wlan0` and `rmnet_data2` each have an SFQ `12:` child under PPQ class `2:2`.
Both `tc -s -d qdisc show` snapshots reported zero bytes, zero packets, zero
drops and zero backlog for that child. Normal captured traffic traversed other
classes, including the WLAN `tsd 14:` path. Merely having an SFQ qdisc attached
does not prove the affected enqueue/drop path is reached.

## Commit coherence

| commit | purpose | depends_on | superseded_by | already_present_in_r53 | required_for_complete_fix | runtime_reachable | include |
|---|---|---|---|---|---|---|---|
| controlled `5daaa53d10e73b3aaebf009be294d7b5b0de2189`; stable `d1bc80da75c789f2f6830df89d91fb2f7a509943`; upstream `82ffbe7776d0ac084031f114167712269bf3d832` | Correct `sfq_drop()` tail unlinking after GSO requeue so `q->tail` cannot retain a stale slot | none beyond the existing SFQ GSO code | none in the audited project series | no; B07 still has the unconditional tail assignment | yes for the GSO-tail issue only | compiled; attached but no traffic observed | no for B08 |
| controlled `6772e40c29026ddeec561557397bd936e781c350`; stable `f9b97d466e6026ccbdda30bb5b71965b67ccbc82`; upstream `7ca52541c05c832d32b112274f81a985101f9ba8` | Validate `perturb_period` before multiplication by `HZ` in `sfq_change()` | none functionally; its project commit follows the GSO-tail commit only by history | none in the audited project series | no; B07 still multiplies the unchecked value | yes for the perturb-validation issue only | config path is present, but normal OxygenOS settings use a valid 10-second period | no for B08 |

The commits touch different functions and correct independent failure modes.
Neither is a follow-up repairing an assumption introduced by the other. They
must not be described as an inseparable two-commit repair series. A future SFQ
stage may qualify them separately or explicitly justify a same-subsystem batch.

## External contract

Both fixes are internal to `net/sched/sch_sfq.c`. They change no UAPI, exported
symbol, shared header, module CRC, firmware interface, device tree, config, or
stock-DLKM ownership. The reason for deferral is runtime evidence and clean
attribution, not a static ABI/KMI blocker.

## Future physical test design

Before any SFQ candidate is selected, prove that real traffic increments the
specific SFQ child counters without replacing the persistent OxygenOS qdisc
policy. Capture `tc -s -d qdisc show` before, during, and after parallel large
TCP/GSO flows plus latency-sensitive pings. Require rising packet/byte counters
on `sfq 12:`, sane drop/overlimit/backlog accounting, WLAN reconnect, handoff,
cellular regression, deep idle, and a kernel/network error scan. If traffic
continues to bypass class `2:2`, SFQ remains no-current-value for Canoe.

Raw B07 evidence is retained at
`out/oos1610500-custom-r53-b08-audit/live-b07-qdisc.txt`.

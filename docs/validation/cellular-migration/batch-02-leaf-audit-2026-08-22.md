# Cellular Batch 02 leaf audit

Date: 2026-08-22

Baseline branch: `experiment/cellular-batch-01`

Physically validated Batch 01 vendor-DLKM SHA-256:
`640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958`

Kernel contract: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`

Result: **DEFER — NO BATCH-2 LEAF CONTAINS A REAL `.102` RUNTIME CHANGE**

## Decision

None of the six permitted leaf candidates satisfies the mandatory actual
runtime behavior-change gate. A source-built replacement would rebuild `.097`
runtime code under a `.102` package label, repeating the Batch 01 delivery and
signing proof without exercising newer behavior. No Batch 02 experiment branch,
module, vendor-DLKM image, or physical-test command was created.

The complete machine-readable comparison and scores are in
[batch-02-leaf-ranking.tsv](batch-02-leaf-ranking.tsv).

## Authoritative source comparison

The comparison is between the exact OnePlus source snapshots:

- shipping CPH2747 OOS 16.0.9.400(EX01):
  `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`, datarmnet-ext `.097`;
- newer same-SM8850 snapshot:
  `d447f713d6403f707a2910383495f4ada98cfa4d`, datarmnet-ext `.102`.

The current Batch 01 worktree has zero changes from the shipping snapshot in
all six leaf directories. Between `.097` and `.102`, exactly these files
change:

```text
aps/define_aps.bzl
offload/define_offload.bzl
perf/define_perf.bzl
perf_tether/define_perf_tether.bzl
shs/define_shs.bzl
wlan/define_wlan.bzl
```

Each change only renames the Kleaf selector namespace from
`//build/kernel/kleaf:socrepo_{true,false}` to
`//build/qcom_build_extensions:qtisocrepo_{true,false}`. Module names, source
lists, dependencies, compiler options, and all C/H/Kconfig inputs are unchanged.
The newer selector labels are not present in the frozen g6744 build tree, so
copying the `.102` definition verbatim would be a build-system mismatch with no
runtime value.

The non-selector Git object manifests prove exact identity:

| Leaf | Files checked | `.097` manifest SHA-256 | `.102` manifest SHA-256 | Result |
| --- | ---: | --- | --- | --- |
| `rmnet_aps` | 12 | `81f7cb71fb1eb6f1f47b63ff5fa53e73dcbc23bd98294035661faa2847f05be6` | `81f7cb71fb1eb6f1f47b63ff5fa53e73dcbc23bd98294035661faa2847f05be6` | IDENTICAL |
| `rmnet_offload` | 19 | `e25eaf0092df715019529fe44936cc6f0e6e4bff5a763728740736c3abaadf9b` | `e25eaf0092df715019529fe44936cc6f0e6e4bff5a763728740736c3abaadf9b` | IDENTICAL |
| `rmnet_perf` | 14 | `8f3e0658d196a4e973473393d99455fc2d621c185316b94a2791a5e5ac1250d4` | `8f3e0658d196a4e973473393d99455fc2d621c185316b94a2791a5e5ac1250d4` | IDENTICAL |
| `rmnet_perf_tether` | 6 | `60cee17c05e2452d0bc46fa809a8d53ca8dcdc53f72b3b97c115e1a30aa8859b` | `60cee17c05e2452d0bc46fa809a8d53ca8dcdc53f72b3b97c115e1a30aa8859b` | IDENTICAL |
| `rmnet_shs` | 26 | `dacf1174ff17e95247513ed2b697f17831cb9ecba8e2b0d47c4aefe69b19c5b3` | `dacf1174ff17e95247513ed2b697f17831cb9ecba8e2b0d47c4aefe69b19c5b3` | IDENTICAL |
| `rmnet_wlan` | 17 | `9a7c0efe668b4b83a128a2fede138c7a24d3db48bedb2f7782d442ac523ef638` | `9a7c0efe668b4b83a128a2fede138c7a24d3db48bedb2f7782d442ac523ef638` | IDENTICAL |

Because the runtime sources, DDK source lists, compiler options, and provider
dependencies are identical, each `.102` leaf has the same import, export, and
module-parameter contract as its shipping counterpart. All six export zero
symbols. The exact symbol names and CRCs remain recorded in
[current-cellular-closure.tsv](current-cellular-closure.tsv); the direct
provider edges are in [dependency-edges.tsv](dependency-edges.tsv).

## Leaf ranking

All behavioral-value scores are 1/5 because none contains newer runtime code.
The ordering is therefore a hypothetical least-risk order for a future drop,
not authorization to build one now.

| Rank | Module | Value | Dependency risk | Cellular risk | WLAN/tether coupling | Rollback | Reason |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | `rmnet_aps` | 1 | 2 | 2 | 1 | 5 | Cleanest policy leaf, but no `.102` runtime delta |
| 2 | `rmnet_perf` | 1 | 2 | 3 | 2 | Leaf with three RMNET-core imports, but no `.102` runtime delta |
| 3 | `rmnet_perf_tether` | 1 | 2 | 3 | 4 | Small contract but directly affects tethering |
| 4 | `rmnet_offload` | 1 | 2 | 4 | 3 | Active flow/offload path has broader regression potential |
| 5 | `rmnet_shs` | 1 | 4 | 4 | 138 imports, WALT dependency, 70 module parameters |
| 6 | `rmnet_wlan` | 1 | 3 | 4 | 5 | Crosses the physically qualified WLAN/handoff boundary |

All six would be individually reversible as a vendor-DLKM-only replacement,
so rollback ease is 5/5. That operational convenience cannot satisfy the
actual-change gate.

## RMNET priority fix

The real `.102` behavior change previously identified is in:

```text
vendor/qcom/opensource/datarmnet/core/rmnet_map_data.c
rmnet_map_v5_check_priority()
```

Its line provenance in the newer branch is the OnePlus/QCOM synchronized
snapshot commit `bc8d91d1e146be96d2e27bebe8f753f82bdebeee`; the final comparison
snapshot is `d447f713d6403f707a2910383495f4ada98cfa4d`. No separate logical upstream
fix commit is available in this source history, so the synchronized snapshot is
recorded as provenance rather than inventing a standalone fix identity.

The new branch handles an APS low-latency-bearer packet when the normal
`!tso && low_latency` branch is not active and
`rmnet_ll_get_ipa_ready_status()` reports anything other than
`RMNET_LL_PIPE_SUCCESS`. If the skb carries `RMNET_APS_LLB`, the code sets the
QMAP v5 priority bit so Q6 can prioritize it on the regular pipe. The helper is
also called for TSO headers, making the unavailable-low-latency-pipe condition
relevant to both checksum and segmented uplink paths.

This path is build- and device-reachable: `rmnet_core`, `rmnet_aps`, and the
other policy leaves are loaded on the physically validated CPH2747; the active
data plane uses RMNET over IPA; and the data-format priority gate is compiled
into the loaded core. The exact unavailable-LL-pipe plus APS-marked-packet
condition was not observed during Batch 01, so no physical benefit is claimed.
It can realistically occur during low-latency pipe unavailability, including
initialization or recovery windows, but that remains a test hypothesis.

The fix belongs to `rmnet_core.ko`, not any permitted Batch 02 leaf. Replacing
that provider crosses the six leaf-consumer contracts and the broader RMNET
export boundary. It therefore belongs to the later coherent RMNET-provider
migration phase, which this task explicitly excludes.

## Provider and release contracts

For all six rejected leaves, the `.102` direct RMNET-core imports are identical
to `.097` and would resolve with identical CRCs against the exact stock
`rmnet_core`. No newer IPA/GSI provider is required by these unchanged leaf
sources. `rmnet_shs` retains its existing WALT provider dependency, and
`rmnet_wlan` retains its WLAN-facing runtime hooks; neither has a `.102` source
change that justifies reopening those boundaries.

No kernel-producing input was changed, so the frozen g6744 release contract is
not being reused for a new artifact. There is no candidate on which to run the
release guard, dist, ABI, KMI, signing, filesystem, or AVB gates. The last
validated Batch 01 contracts remain historical evidence, not a substitute for
a nonexistent Batch 02 build.

## Stop decision

No module was selected. No `experiment/cellular-batch-02-*` branch was created,
no source was imported, no module was built, no vendor-DLKM was assembled, and
no TWRP physical-test plan was issued.

The next valid choices are:

1. keep the current Batch 01 ownership boundary and wait for a newer source
   drop containing an actual isolated leaf fix; or
2. explicitly authorize the later coherent `rmnet_core` provider migration and
   its full consumer/reachability test plan.

Neither option is performed here.

# RMNET core .102 source delta

Date: 2026-08-22

## Provenance and scope

- Shipping source baseline: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- Newer source snapshot: `bc8d91d1e146be96d2e27bebe8f753f82bdebeee`
- Matching newer-drop object also present at: `d447f713d6403f707a2910383495f4ada98cfa4d`
- Runtime source object: `vendor/qcom/opensource/datarmnet/core/rmnet_map_data.c`
- Imported object ID: `83b683afb1b2efbeb1b82c728f327abd41d522ae`

The complete old-to-new `datarmnet` diff contains one runtime source hunk and
one build-selector-only `define_modules.bzl` change. Only the exact runtime
source object was imported. The newer selector labels were not imported
because they describe the newer repository's build namespace, not RMNET
runtime behavior.

All 21 public and internal `datarmnet` headers are byte-identical between the
two source snapshots.

## Changed runtime function

| File | Function | Old behavior | New behavior | Canoe reachability | Expected effect | Security relevance | Provider/consumer impact |
|---|---|---|---|---|---|---|---|
| `core/rmnet_map_data.c` | `rmnet_map_v5_check_priority()` | An APS LLB mark set QMAP priority only in the checksum-header low-latency path (`!tso && low_latency`). Other paths left the QMAP priority bit clear. | When the IPA low-latency pipe state is not `RMNET_LL_PIPE_SUCCESS`, an APS LLB-marked packet also receives the normal QMAP priority bit and increments `ul_prio`; the driver emits `priority bit set`. | Compiled into the Canoe `rmnet_core.ko`; called from both QMAP v5 checksum and TSO uplink handling when priority-format processing is active. | Preserve modem-side QMAP priority for APS LLB traffic when the dedicated IPA low-latency pipe is unavailable. | No security or memory-safety claim. This is a data-path priority correctness change. | Internal function only. No header, prototype, export, CRC, enum value, or structure layout changes. |

## Priority inputs and IPA state

The APS state is encoded in `skb->priority`:

- `RMNET_APS_MAJOR = 0x9B6D`
- `RMNET_APS_LLC_MASK = 0x0100`
- `RMNET_APS_LLB_MASK = 0x0200`

The new branch specifically consumes `RMNET_APS_LLB(skb->priority)`.

The low-latency pipe state is read through
`rmnet_ll_get_ipa_ready_status()`. It is initialized to failure, then updated
by the `ipa_register_rmnet_ll_cb()` registration path to:

- `RMNET_LL_PIPE_SUCCESS = 0`
- `RMNET_LL_PIPE_FAILED = -1`
- `RMNET_LL_PIPE_FAILED_ENXIO = -ENXIO`

The new behavior runs only when the existing checksum low-latency case was not
taken and the IPA low-latency status is not success.

## Binary proof

The exact generated unsigned module was compared to the Batch-01 stock core.
Within `rmnet_map_v5_checksum_uplink_packet`:

- stock core: no relocation to `rmnet_ll_get_ipa_ready_status`
- candidate core: `R_AARCH64_CALL26 rmnet_ll_get_ipa_ready_status` present
- stock core: `priority bit set` diagnostic absent
- candidate core: `priority bit set` diagnostic present

The fail-closed report is generated at
`out/cellular-rmnet-core-102-candidate/rmnet-core-priority-binary-proof.txt`.

## Runtime evidence boundary

Existing physical-validation captures contain successful IPA/RMNET
low-latency client registration, but no `priority bit set` event. Therefore:

`RMNET_CORE .102 PRIORITY-FIX TRIGGER NOT OBSERVED`

The candidate can be tested for compatibility, but the behavioral path must
not be claimed as physically exercised unless the unavailable-low-latency-pipe
condition and an APS LLB-marked packet are both observed or induced safely.

# ColorOS 17.0.0.102 RMNET private-ABI boundary review

Review date: 2026-10-30. Source baseline:
`efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c`, branch
`experiment/coloros17-sm8850-dlkm-updates`.

This is a read-only source/ELF boundary audit, not a successful replacement
module qualification. No implementation, configuration, symbol versions,
device partitions or module-loading behavior were changed by this review.
The existing released boot-only artifacts remain unchanged.

## Scope and evidence limits

The target is ColorOS `CPH2745_17.0.0.102(EX01)`, not an older OxygenOS `.601`
module population. Its framework Android 17 / vendor Android 16 split does not
by itself establish a need to change the Android 16 KMI6 driver contract.

The exact shipping source archive is unavailable. Public source labels,
version strings, matching filenames, clean patch application and matching
export CRCs are not proof of the private runtime ABI. No further archive
request is a prerequisite to this review; the next gate is a per-component
unchanged-control build and direct binary/layout comparison.

The [module inventory](coloros17-module-inventory.md) and its TSVs identify
the exact stock files. Local read-only ELF inputs are under
`/home/travis/Android/kernel-test-backups/oos1700102-baseline-20261025-120000/modules/vendor_dlkm/lib/modules/`.
These are extracted stock files, not the public-source rebuild.

| Stock input | SHA256 | Direct module dependencies |
|---|---|---|
| `rmnet_core.ko` | `220a9dadafa4250ead10b78a9195399aacbe11934ff9558ffd142b45157ed8c2` | `ipam,rmnet_mem,rmnet_ctl,qmi_helpers` |
| `rmnet_ctl.ko` | `d7b904e503ddddcbed137c66bab3a078142d915401cfca801cff65428eba0d5b` | `ipam,qcom_ipc_logging` |
| `rmnet_mem.ko` | `0f9c1f4128da06d9e4b697fe045522e705f98bc1746ed5a771440df3981d8239` | none |
| `ipam.ko` | `f738a3ba38f38d0979f625f2b0c75fe7bc98069ef3442903218b05aee6a8a625` | See complete inventory; includes `rmnet_mem`, not `rmnet_core` |

Six stock modules directly depend on `rmnet_core`: `rmnet_aps`,
`rmnet_offload`, `rmnet_perf`, `rmnet_perf_tether`, `rmnet_shs`, and
`rmnet_wlan`. All ten modules in this closure have neither BTF nor DWARF.
Thus the complete stock private structure layouts cannot be recovered from
their available type-debug metadata. Existing common-kernel/GKI BTF can
constrain common kernel types, but is not evidence for module-private RMNET
layout. Instruction-derived offsets below prove individual constraints only.

Paths in the following sections are relative to
`vendor/qcom/opensource/` unless otherwise stated.

## Actual cross-module boundary map

| Boundary / headers | Runtime stock consumers | Fields, signatures and ownership requiring agreement |
|---|---|---|
| `datarmnet/core/rmnet_config.h:44`, `:128`, `:223`: `rmnet_shs_clnt_s`, `rmnet_port`, `rmnet_priv` | `rmnet_shs`, `rmnet_offload`, `rmnet_aps` | `netdev_priv()` exposes core-owned private data without naming it in the net-device API. SHS reads `priv.mux_id/real_dev`, `port.data_format`, `port.stats.pb_marker_seq`, and SHS `config/map_mask`; offload obtains `priv.real_dev`, retains the core port and registers callbacks; APS casts `priv.aps_cb[16]` to its own timestamp/flow-list structure. |
| `datarmnet/core/rmnet_private.h:52`: `rmnet_skb_cb` | `rmnet_shs`, `rmnet_perf`; packets also pass other hook consumers | `skb->cb` is reinterpreted as `flush_shs`, `qmap_steer`, `tethered`, `coal_bytes`, `coal_bufsize`, `bif`, `ack_thresh`, `ack_forced`. The comment requires the first entries to retain their legacy position. Generic `sk_buff *` signatures do not expose this private contract. |
| `datarmnet/core/rmnet_descriptor.h:36`: `rmnet_frag_descriptor`, `rmnet_fragment` | `rmnet_offload`, `rmnet_aps` | Core allocates descriptors; offload directly reads/writes their `list`, `dev`, `coal_bufsize/coal_bytes`, `len`, `hash`, `gso_size/gso_segs`, `ip_len/trans_len`, `ip_proto/trans_proto`, `tcp_seq/tcp_flags`, and validity/ownership-related bitfields. Embedded-list, inline fragment access and `container_of()` assumptions must agree. |
| `datarmnet/core/rmnet_module.h:39`, `rmnet_hook.h` | All six extensions | Registration transports `{int hooknum; void *func;}`. Exact hook numbers, typed callback prototypes, calling convention, return conventions and KCFI IDs are runtime ABI erased from the registry's callback field type. Export CRC equality for registration alone cannot establish callback safety. |
| `datarmnet/core/rmnet_map.h:186/:218`: `rmnet_map_pb_ind`, `rmnet_map_dl_ind` | `rmnet_shs`, `rmnet_offload` | Extensions allocate/register callback objects; core traverses their embedded lists and invokes callbacks. Priority, handler and list offsets, wire-header interpretation and unregister/RCU ownership must remain compatible. |
| `datarmnet/core/qmi_rmnet.h:24`: `qmi_rmnet_ps_ind` | `rmnet_shs`, `rmnet_offload` | Two `void (*)(void *port)` callbacks plus embedded list are supplied by the extensions and invoked by core. `qmi_info`/`qos_info` opaque getters have public declarations, but no direct stock extension import of those getters was found in this closure; their source casts are internal to core. |
| `datarmnet/core/rmnet_ctl.h`: `rmnet_ctl_client_hooks`, `rmnet_ctl_client_if` | `rmnet_core` consumes stock `rmnet_ctl` | Core uses the exported interface table's `reg`, `dereg`, `send`, `log` callbacks, register hook, handle lifetime and SKB ownership. `rmnet_ctl_client.h`'s `rmnet_ctl_dev` and its enclosing `rmnet_ctl_ipa_dev` stay inside the separately linked `rmnet_ctl` module. |
| `dataipa/drivers/platform/msm/include/linux/ipa.h:1823`, `:1878`, `:1918` | Core/ctl consume stock `ipam` | IPA ready/stop/RX callbacks and opaque user-data pointers cross the boundary. CTL RX data is treated as an SKB using the `0xFFFFFFFF` length sentinel. LL RX interprets event 1 as SKB receive and event 2 as flow notification. Preserve callback types, event values and packet ownership. |
| `datarmnet-ext/mem/rmnet_mem.h` | `rmnet_core`, `ipam` consume stock `rmnet_mem` | API exchanges `page *`, scalar order/client/result values and standard notifier blocks. Current `rmnet_mem_cb()` ignores `void *data`; IPA's notifier ignores `void *ptr` and consumes the `unsigned long pool_size`. Core supplies `NULL` for powersave and forwards event/data from its handler. No module-private payload structure was found crossing this boundary. |
| `datarmnet/core/rmnet_qmap.h`, `rmnet_map.h` | perf, perf_tether, APS; SHS/offload marker consumers | Packed QMAP command/packet headers and command IDs are a wire/firmware contract independent of C export CRCs. Preserve packing, endianness, bitfields, command values and SKB lengths. |

The top-level `datarmnet/core/Kbuild` links configuration, descriptor,
ingress, map, module-hook, VND, LL-IPA and DFC/QMAP objects into
`rmnet_core.ko`. It links `rmnet_ctl_client.o` and `rmnet_ctl_ipa.o` into the
separate `rmnet_ctl.ko` on Canoe. The in-tree MHI alternatives are not the
selected Canoe transport. `rmnet_ll_endpoint`/buffer/client-ops structures
therefore remain internal to core for the selected IPA transport.

Do not classify arbitrary IPA `netdev_priv()` calls as RMNET-private accesses:
`dataipa/.../ipa_v3/rmnet_ipa.c` uses its own `ipa3_wwan_private` structure,
and RNDIS code has separate private storage.

### Remaining header coverage and non-boundaries

`rmnet_handlers.h` is included by SHS/offload and describes delivery/RX
entry points using SKBs/ports plus `rmnet_packet_context`; those exported
entry points and enum values need normal CRC/calling-convention validation.
`rmnet_trace.h` exposes tracepoints used by stock SHS. Their true exported
tracepoint/iterator objects and callback signatures must be included in the
module oracle, not mistaken for private data returned by a getter.

`rmnet_qmap.h` transitively includes `qmi_rmnet_i.h`, but a source search of
the extension/IPAM closure found no external dereference/instantiation of
core's `qmi_info`, `qos_info`, `rmnet_endpoint`, aggregation-state or LL-core
types. Their mere inclusion is not evidence of an external field consumer.
Core's `rmnet_qmi.h`, `dfc_defs.h`, `dfc.h`, `wda.h`, `rmnet_genl.h`,
`rmnet_vnd.h`, and the selected `rmnet_ll.h`/`rmnet_ll_core.h` have no
additional direct cross-module private-object consumer identified in the
inspected extension/IPAM source. The internal source review still matters
for allocation/initialization/teardown correctness, and the shipping
disassembly/provider oracle must validate the actual built closure.

### Direct source access examples

- SHS: `datarmnet-ext/shs/rmnet_shs_config.c:124` obtains private `real_dev`,
  then a core port; `:148` reads `data_format`. `rmnet_shs_main.c:2060`
  reads private `mux_id`; `rmnet_shs_wq_mem.c:154` reads the port's
  `stats.pb_marker_seq`. `rmnet_shs_common.c:25` uses the private SKB CB.
- APS: `datarmnet-ext/aps/rmnet_aps_main.c:94/:103` defines the overlay;
  `:824` checks its size against the core's 16-byte `aps_cb`, which does not
  independently prove the stock offset or stock allocation size.
- Offload: `datarmnet-ext/offload/rmnet_offload_state.c:162` obtains the core
  port through private `real_dev`; `:62` registers PS/DL callback structures.
  `rmnet_offload_main.c:96` directly dissects the core descriptor, including
  bitfields and protocol/header-length fields.
- Perf: `datarmnet-ext/perf/rmnet_perf_tcp.c:236/:342/:493` and
  `rmnet_perf_udp.c:271` write the SKB CB later consumed by core/SHS.
- CTL: `datarmnet/core/rmnet_ctl_ipa.c:23/:42/:74` supplies IPA callbacks and
  opaque data; `rmnet_ctl_client.c` invokes the registered core receive hook.
- Memory: `datarmnet-ext/mem/rmnet_mem_main.c:137` supplies scalar pool size
  with a `NULL` notifier payload; `dataipa/.../ipa_v3/ipa_dp.c:1441` consumes
  that scalar. The exported private notifier variable has no direct import
  by the inspected stock core/IPAM modules; they use the API functions.

## Hook contract inventory

The following order is the source's numeric registration ABI, starting at
zero. `rmnet_module_hook_*` exported wrappers additionally handle an optional
result-output pointer for non-void callbacks; that wrapper signature is not
the same thing as the registered callback prototype below.

| IDs | Registered callbacks and exact argument/return shape | Main consumer |
|---|---|---|
| 0, 1 | `offload_ingress(list_head *, rmnet_port *) -> void`; `offload_chain_end(void) -> void` | offload |
| 2, 3, 4 | `shs_skb_entry`, `shs_skb_ll_entry`, `shs_switch`: `(sk_buff *, rmnet_shs_clnt_s *) -> int` | SHS |
| 5, 6, 7 | `perf_tether_ingress(tcphdr *, sk_buff *) -> void`; `perf_tether_egress(sk_buff *) -> void`; `perf_tether_cmd(u8,u64) -> void` | perf_tether |
| 8, 9, 10 | `perf_ingress(sk_buff *) -> int`; `perf_egress(sk_buff *) -> void`; `perf_set_thresh(u32,u32) -> void` | perf |
| 11, 12 | `aps_pre_queue(net_device *, sk_buff *) -> void`; `aps_post_queue(net_device *, sk_buff *) -> int` | APS |
| 13 | `wlan_flow_match(sk_buff *) -> void` | WLAN |
| 14, 15, 16 | `aps_data_inactive(void) -> void`; `aps_data_active(rmnet_frag_descriptor *, sk_buff *) -> void`; `aps_data_report(sk_buff *) -> void` | APS |
| 17, 18, 19 | `perf_ingress_rx_handler(sk_buff *) -> void`; `wlan_ingress_rx_handler(sk_buff **) -> rx_handler_result_t`; `perf_cmd_ingress(sk_buff *) -> void` | perf/WLAN |
| 20, 21, 22, 23, 24 | `perf_coal_common_stat(uint8_t,uint32_t) -> void`; `perf_coal_stat(uint8_t,uint8_t,uint64_t,uint32_t) -> void`; `perf_seg_stat(uint8_t,sk_buff *) -> void`; `perf_non_coal_stat(uint8_t,uint64_t) -> void`; `perf_ecn_ingress(sk_buff *) -> int` | perf |

Stock instruction evidence independently confirms registration stride 16,
index at offset 0, callback at offset 8, and maximum valid index 24. KCFI
checks are present before the indirect callback. Two inspected examples are:

- `rmnet_module_hook_shs_skb_entry`: expected callback KCFI ID
  `0xb831bc97`; the stock `rmnet_shs_assign`/`rmnet_shs_ll_handler`
  `__kcfi_typeid_*` symbols also have that value.
- `rmnet_module_hook_aps_data_active`: expected callback KCFI ID
  `0x9d44cc76`.

All 25 stock wrapper call sites were additionally decoded. Values below are
the callback type IDs checked immediately before the indirect call, recovered
from the two `movk w17` immediates following the function-prefix type-ID load.
They are not the type IDs of the exported wrapper functions themselves.

| Callback | Stock expected KCFI ID |
|---|---|
| `offload_ingress` | `0x1e5595a5` |
| `offload_chain_end` | `0xe5c47d60` |
| `shs_skb_entry` | `0xb831bc97` |
| `shs_skb_ll_entry` | `0xb831bc97` |
| `shs_switch` | `0xb831bc97` |
| `perf_tether_ingress` | `0x9abbf7a0` |
| `perf_tether_egress` | `0x121b693e` |
| `perf_tether_cmd` | `0x99cb7481` |
| `perf_ingress` | `0xb394cb1c` |
| `perf_egress` | `0x121b693e` |
| `perf_set_thresh` | `0xfea22452` |
| `aps_pre_queue` | `0x55675227` |
| `aps_post_queue` | `0xdd93dde9` |
| `wlan_flow_match` | `0x121b693e` |
| `aps_data_inactive` | `0xe5c47d60` |
| `aps_data_active` | `0x9d44cc76` |
| `aps_data_report` | `0x121b693e` |
| `perf_ingress_rx_handler` | `0x121b693e` |
| `wlan_ingress_rx_handler` | `0x85977171` |
| `perf_cmd_ingress` | `0x121b693e` |
| `perf_coal_common_stat` | `0xb9dfcb48` |
| `perf_coal_stat` | `0xbae1b621` |
| `perf_seg_stat` | `0xd07bdb20` |
| `perf_non_coal_stat` | `0x99cb7481` |
| `perf_ecn_ingress` | `0xb394cb1c` |

All 25 callbacks still require comparison in an actual rebuilt control.
Matching KCFI IDs establish a necessary callback-type check, not complete
private object layout, lifetime, return-value or ownership semantics.

## Stock instruction-derived layout constraints

The following were read from `llvm-objdump -dr --disassemble-symbols=...`
against the exact original stock modules. These constraints are distinct
from a complete type-size/member-offset proof.

| Stock module / function | Observed constraint | What is and is not proved |
|---|---|---|
| core / `rmnet_get_real_dev` | 64-bit load at port + `0` | Matches source's `port.dev`; proves this accessed field only. |
| core / `rmnet_get_endpoint` | `muxed_ep` base port + `0x10`, 8-byte indexed slots; endpoint mux ID at `0`, hlist node at `0x10` | Establishes these accessed offsets and slot stride, not full port size. |
| core / `rmnet_get_qmi_pt` | 64-bit load at port + `0x9a8` | Expected private `qmi_info` access. Does not prove preceding embedded aggregation-state layout. |
| core / `rmnet_get_rmnet_port` | private real-device pointer load at **net-device-relative** `0xa88`; real device's handler/data at `0x108/0x110` | Do not turn `0xa88` into a `rmnet_priv`-relative offset until the actual private-data base has been independently proved. |
| core / `rmnet_get_qos_pt` | 64-bit load at **net-device-relative** `0xc60` | Source identifies `qos_info`; complete `rmnet_priv` size and member layout are unproved. |
| APS / `rmnet_aps_notifier_cb` | timestamp byte write at **net-device-relative** `0xc68` | Source identifies `aps_cb[0]`; does not establish the whole private-data allocation size. |
| SHS / `rmnet_shs_switch_hook_entry` | byte read/write at **SKB-relative** `0x29` | Source identifies `qmap_steer`; CB base must be independently proved before deriving a CB-relative offset. |
| core / `rmnet_frag_pull` | descriptor length load at `0x30`, fragment list at `0x10` | Proves these two descriptor constraints, not every bitfield or the complete object size. |
| core / `rmnet_module_hook_register` | record stride `0x10`, index `+0`, callback `+8`, valid maximum `0x18` | Establishes registry record/number constraints; not all callback signatures. |

Complete stock equivalence is currently **UNPROVEN** for `rmnet_priv`,
`rmnet_port` and embedded stats/aggregation state, `rmnet_frag_descriptor`
including bitfields, and the callback-containing marker objects. There is
no basis to manufacture missing offsets from these partial observations.

## Relationship to the proposed MAP bounds fix

The official Qualcomm candidate is
`ea47e85e97b72674a30e5846975c5a1eb04c6fde`,
`datarmnet: Fix OOB read and length underflow in MAP coalesced packet ingress`.
The [shipping binary review](coloros17-stage-a-binary-review.md) separately
records which guards are missing in the original module. Source applicability
and binary evidence must not be conflated.

Its patch changes only `core/rmnet_descriptor.c`, `core/rmnet_handlers.c`, and
`core/rmnet_map_data.c`. It does not modify shared headers, exported
declarations, hook enumeration, structure definitions or callback signatures.
Its functional intent is to reject malformed lengths before segmentation or
delivery, in both linear and fragment processing paths. Consequently there
is no direct new private-layout change in this patch; nevertheless a rebuilt
provider is unsafe to substitute until the *starting source/control* agrees
with the stock consumers at the actual boundaries described above.

## Next gate: unchanged control and narrowly scoped probes

Before applying the candidate or calling a replacement deployable:

1. Build the unchanged `rmnet_core` and only the required control dependency
   closure with the exact selected Canoe configuration/toolchain. Record all
   build inputs and module hashes. Do not enable features to make this match.
2. Compare executable/runtime-data sections and normalized relocation
   targets/addends against stock. Section equality, where achieved, is much
   stronger evidence than matching CRCs; document excluded non-runtime
   metadata rather than silently ignoring runtime differences.
3. If executable equality is not obtained, use the rebuilt object's DWARF
   for exact candidate sizes/alignment/all relevant offsets, and compare
   stock instructions for **every externally accessed** field, allocation
   size, bitfield mask, list/container calculation and callback table.
   Read-only compile probes may be necessary when optimized DWARF omits a
   field; generated probe outputs are evidence, not source authority.
4. Compare all 25 hook numeric IDs, actual callback argument/return shapes
   and KCFI type IDs; marker/PS/CTL/IPA callbacks and ownership require
   their separate checks. A cast or forced type/CRC record is not a repair.
5. Run the full true-export/import/CRC/namespace/protected/signature oracle
   with the actual replacement/stock provider precedence, including stock
   extension modules and the unchanged `ipam`, `rmnet_mem`, `rmnet_ctl`
   providers. Loader ordering must not choose an old duplicate accidentally.
6. Stop at any unknown critical private edge. A source-only backport can be
   recorded as useful but not deployable until its boundary is established.

The following unchanged-control review advances that initial gate. It does
not establish deployability by itself or authorize implementation changes.

## Unchanged RMNET control: private-boundary checks

The build review supplied the unchanged-source Canoe control at:

`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/control-rmnet-core/rmnet_core/run01/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`

SHA256:
`940a68ddaeed81f7fd46ca8f4f064c8f5064e41108d94c56077294473ac5aacc`.
Compilation, link and modpost passed in that control build. This object
retains DWARF; the shipping object does not. `pahole` and focused instruction
comparison give the following candidate layouts and stock agreement:

| Candidate type | DWARF size / selected offsets | Independent stock agreement |
|---|---|---|
| `net_device` | size/private-data member base 2688 (`0xa80`), alignment 64; handler/data `0x108/0x110` | Five unchanged core getters have identical instruction bodies/relocations. Stock absolute private accesses agree with the candidate base plus the private member offsets below. This does not infer a base by subtracting unproven offsets. |
| `rmnet_priv` | size 504; `mux_id=0`, `real_dev=8`, `pcpu_stats=16`, `gro_cells=24`, `stats=32`, `qos_info=480`, `aps_cb=488` | Stock `rmnet_link_ops` data at its `priv_size` member contains 504; core getters and stock APS access match the candidate real/QOS/APS locations. |
| `rmnet_port` | size 2816; `dev=0`, `data_format=8`, `muxed_ep=16`, `bridge_ep=2056`, `rmnet_perf=2064`, `agg_state=2072`, `qmi_info=2472`, `stats=2496`, `shs_cfg=2768`, `phy_shs_cfg=2784`, pool lock/pointer `2800/2808` | Stock port allocation requests `0xb00` bytes. Core's port-related access instructions match; the QMI getter independently loads `0x9a8` (2472). |
| `rmnet_frag_descriptor` | size 88; `list=0`, `frags=16`, `dev=32`, `len=48`, header protocol bytes `78/79`, flags byte `81` | Stock allocation requests `0x58` bytes; descriptor field/bitfield access code is unchanged outside the separately explained generic header code generation below. |
| `rmnet_skb_cb` | size 24; `flush/qmap/tethered=0/1/2`, coal counts `4/8`, `bif/ack_thresh/ack_forced=12/16/20` | Candidate DWARF places `sk_buff.cb` at 40; stock SHS's absolute `0x29` qmap-steer access agrees. No arbitrary private-base assumption is used. |
| `rmnet_shs_clnt_s` | size 16; config/map/max `0/2/4`, port pointer `8` | Core callback/data-layout code matches. |
| `rmnet_endpoint` | size 32; mux/device/list `0/8/16` | Exact unchanged endpoint getter and list-offset constraints. |
| DL / PB / PS callback objects | sizes 40/32/32; DL handlers `8/16`, list `24`; PB handler/list `8/16`; PS handlers `0/8`, list `16` | Core registration/traversal/call code matches; no changed callback/private-field code was found. |
| `rmnet_ctl_client_if` | size 32; `reg/dereg/send/log=0/8/16/24` | Shape inspected in the control DWARF; the separate CTL provider/load-policy gate still applies. |

All **25/25** registered callback KCFI checks in the control match the exact
stock values already listed above. The build review additionally records
140 matching exports, 56 matching retained stock-consumer edges and normal
resolution/versioning of the control's 155 ELF imports / 156 version records.
Those numbers are component-control results, not a replacement for a full
production-package oracle.

### All remaining control code-generation differences

The build review's complete disassembly diff is retained locally as
`control-rmnet-core/rmnet-core-disassembly.diff` in the same evidence tree.
It identifies only the following instruction/relocation differences; 35
other runtime sections, including callback/data/allocation information,
match. No differing private field access, allocation size, bitfield mask or
callback layout was found in that diff.

1. Nine alternative instructions change `stadd w10,[x9]` to
   `ldadd w10,w12,[x9]`, from existing stable commit
   `5ca91006cceafcb3caccdb268e79b2ffdc83c05a` (upstream
   `535fdfc5a228524552ee8810c9175e877e127c27`). The `.altinstructions`
   relocations map replacement addresses
   `8e8,a14,8928,893c,e8f4,e908,e91c,e930,19294` to original LL/SC loops
   `838,9d0,8788,87dc,e518,e720,e83c,e874,191dc`. Every original loop
   already clobbers `w12` with `ldxr/add/stxr`; the source inline assembly
   declares that temporary early-clobbered and its operation is void-return.
   The new returned old value is unused; no previously live value is
   corrupted. Both instructions retain the same relaxed atomic add and
   replacement length. This is explained generic arm64 code generation,
   not a private RMNET layout change.
2. Two list-validation compares at `14d94` and `151f0` swap equality
   operands. Each feeds only `b.eq`; equality's Z flag is invariant under
   that swap. No other flag-dependent consumer uses the differing
   non-equality flags before overwrite/call. They do not change semantics.
3. Two relocation records at `bea0/beac` change `system_wq` to
   `system_percpu_wq` because current `schedule_work()` names the latter.
   These are **different queue pointers**, not symbol aliases. Current
   common code allocates both as bound queues with flags 0 / max-active 0.
   The affected RMNET aggregation work is synchronized by its own lock,
   `hrtimer_cancel()` and `cancel_work_sync(&state->agg_wq)`; no reliance
   on a global `system_wq` flush was identified in this component. However,
   actual import load policy fails independently, as described next.

Complete original source/debug identity remains unproven, but the unchanged
control now supplies positive actual-boundary evidence beyond CRC matching.
No replacement module has been installed or physically validated.

## Hard stop: frozen-kernel unsigned-import policy

The build review decoded the frozen qualified vmlinux's actual compiled
`permitted_symbol_imports` and `protected_exports`, rather than relying on
current source symbol-list files. Its evidence is
`control-rmnet-core/rmnet-core-load-policy-run03.json` in the local build
evidence tree. There are 10,381 permitted imports and 633 protected exports.
`system_wq` is permitted, but **`system_percpu_wq` is not**.

Consequently the unsigned unchanged control would fail the module loader's
import policy with `-EACCES`, even though that symbol exists and has the
expected CRC. No protected-export collision was found. This is a real
deployment blocker, not a CRC/private-layout failure or permission to weaken
the loader. No kernel symbol-list change, signature workaround or module
source adaptation was performed by this review.

Current result: **PRIVATE/KCFI CONTROL CHECKS PASS; MODULE DEPLOYMENT BLOCKED
BY `system_percpu_wq` IMPORT POLICY**. Any narrowly scoped preservation of
the stock workqueue import requires separate implementation review and a new
component build/full loader-provider validation. No candidate is ready for
flashing or release on the basis of this document.

## Independent review of a narrow legacy-queue adaptation

The following narrow source change was independently reviewed, then applied
by the integration owner in a separate implementation step. The private-ABI
review itself did not edit the driver implementation:

```c
/* rmnet_map_flush_tx_packet_queue(), same work item and execution context */
queue_work(system_wq, &state->agg_wq);
```

This would replace the single `schedule_work(&state->agg_wq)` at
`datarmnet/core/rmnet_map_data.c:1331`. The two relocation differences are
the ADRP/LDR materialization of that one queue pointer, not two source call
sites. The separate stock SHS module has two other `schedule_work()` sites;
SHS is not part of this proposed adaptation and must remain untouched.

### Upstream semantics and security-fix preservation

Current common commit `f5875f3ceb24e6a65ce12bb4c9045c09c08b45f9`, upstream
`a2be943b46b4a7478ea8ddf9bb8e5251c59fceb7`, changes the convenience
`schedule_work()` helper from `queue_work(system_wq, work)` to
`queue_work(system_percpu_wq, work)`. Its purpose is explicit queue naming/API
migration ahead of a future default-affinity transition; it is not a fix for
the RMNET MAP length bug. It is carried as a dependency of workqueue watchdog
overflow hardening, which must remain intact. The preceding `WQ_PERCPU`
addition is current commit `35b2ed533d80a73fbdb7d9514d30a791aaa3199b`,
upstream `930c2ea566aff59e962c50b2421d5fcc3b98b8be`.

In this exact frozen kernel, both `system_wq` and `system_percpu_wq` are
still exported and allocated by `workqueue_init_early()` as
`alloc_workqueue("events", 0, 0)`. They are distinct objects with equivalent
bound-queue flags/default max-active, not aliases. Current `queue_work_on()`
and `__queue_work()` do **not** issue a deprecated-system-queue warning or
redirect this legacy pointer, regardless of the upstream commit message's
general migration description. `queue_work()` calls
`queue_work_on(WORK_CPU_UNBOUND, wq, work)`; because the selected legacy
queue is not `WQ_UNBOUND`, `__queue_work()` selects the current CPU.

Explicitly choosing `system_wq` here therefore preserves the **actual shipping
RMNET module's queue object and affinity behavior**, not just its symbol CRC.
It leaves common workqueue migration, watchdog validation, lifetime fixes,
arm64 atomic fixes and the proposed MAP security bounds checks unchanged.
No common-header or KMI-list rollback is necessary or justified.

### Work-item lifecycle and shared-queue assumptions

- `rmnet_map_tx_aggregate_init():1625-1633` initializes each of the two
  per-port aggregation states, lock, hrtimer and the same `agg_wq` work item.
- `rmnet_map_flush_tx_packet_queue():1325-1332` runs from the hrtimer and
  queues that work without blocking; the boolean enqueue result is unused
  in both forms. No work item allocation, duplicate queue or callback change
  is introduced.
- `rmnet_map_flush_tx_packet_work():1301-1322` derives the same enclosing
  aggregation state, holds `agg_lock`, clears/ships an existing buffered SKB
  and resets aggregation state. The proposed call changes none of that.
- `rmnet_map_tx_aggregate_exit():1654-1658` first stops the hrtimer, then uses
  `cancel_work_sync(&state->agg_wq)` for each state before freeing pending
  aggregation data. Cancellation follows the work item, not a queue-name
  assumption, so using the actual stock queue preserves teardown ordering.
- No `flush_scheduled_work()`, `flush_workqueue(system_wq)` or cross-module
  ordering dependency was found in this core/extension source closure.
  QMI separately flushes its own `rmnet_ps_wq`, which is unrelated.

**Review result:** this one-site source adaptation is a justified narrow
compatibility candidate. Acceptance still requires rebuilding it, proving
the `system_percpu_wq` import disappeared naturally and the stock
`system_wq` import/load policy/CRC is retained, checking the resulting
instruction/relocation delta, rerunning private/export/provider/signature
gates and testing actual networking/aggregation/teardown behavior when
physical testing is separately authorized. No loader bypass, CRC forcing,
kernel policy change or pre-qualification deployment is authorized here.

## Adapted control run04: completed static component gate

The applied one-site legacy-queue adaptation was rebuilt at:

`control-rmnet-core/rmnet_core/run04/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`

under the same local evidence root. SHA256:
`00dcdcac2e1f93600804dd5aec50e360a7359877b0a892a0105afb8bf96a1838`.
Compilation, link and modpost passed; no new kernel Image or dependency
module rebuild was needed for this control. This artifact does not yet
contain the upstream MAP bounds fix.

Independent private/KCFI rechecks:

- All 25 indirect-call KCFI checks exactly match the shipping module.
- Full `pahole` output matches run01 byte-for-byte for 15 boundary types:
  `rmnet_priv` (504), `rmnet_port` (2816), `rmnet_frag_descriptor` (88),
  `rmnet_fragment` (32), `rmnet_frag_descriptor_pool` (24), `rmnet_skb_cb`
  (24), `rmnet_shs_clnt_s` (16), `rmnet_endpoint` (32), `rmnet_map_dl_ind`
  (40), `rmnet_map_pb_ind` (32), `qmi_rmnet_ps_ind` (32),
  `rmnet_ctl_client_if` (32), `rmnet_aggregation_state` (200), `net_device`
  (2688) and `sk_buff` (248). Sizes are bytes, not inferred stock-debug data.
- The five focused core getter instruction bodies remain identical to
  stock, retaining all independently derived offset constraints.
- Actual undefined symbols now contain `system_wq`, not
  `system_percpu_wq`.

The production build review reports 140/140 exports, 156/156 version records,
155/155 actual ELF imports and all 56 retained consumer edges matching stock.
All 5,590 canonical runtime relocations now match stock, including the two
restored legacy-queue relocation records. The frozen compiled import-policy
check has **0 failures and 0 protected-export collisions**. No KMI list,
kernel policy, trusted key or CRC was changed to obtain that result.

The remaining text differences are precisely the previously reviewed nine
arm64 LSE alternatives and two equality-compare operand swaps, not private
layout changes. A single allocation-tag line-number field also moves from
1429 to 1430 because of the added explanatory source comment; the other
allocation-tag fields and actual allocation sizes remain unchanged.

Current result: **ADAPTED CONTROL STATIC PRIVATE/KCFI/PROVIDER/LOAD-POLICY
GATES PASS**. This supports proceeding to the separately reviewed MAP bounds
backport on this component. It does not claim a deployed security fix, exact
original source-archive identity, physical validation or release readiness.
The patched module must be rebuilt and these gates rerun before any package
can be considered a device-test candidate.

## Final source review and run07 private gate

The integration owner separately applied residual guards in
`rmnet_descriptor.c`, `rmnet_map_data.c` and `rmnet_handlers.c` after the
official upstream patch, isolated in commit
`b7c709d8cd1381837afd97f170af6c5b3190a078`. The final source review found no public header,
callback, structure-member, exported-variable or object-ownership change:

- Both coalescing validators use a non-member `unsigned int` packet-count
  accumulator instead of `u8`. Six 8-bit NLO counts can total at most 1530,
  far below the wider type's limit; totals above the protocol limit 48 are
  rejected rather than wrapping. Existing packet loops therefore remain
  within their 8-bit local counters' valid range.
- The descriptor GRO shortcut requires a nonzero packet count, matching
  the linear shortcut's treatment. An empty NLO cannot bypass validation
  into GSO payload subtraction. The separate checksum workaround already
  requires exactly one packet.
- Basic header guards precede 4-byte MAP reads, 8-byte next-header dispatch
  and the fixed 32-byte MAP-plus-coalescing-header access. The normal V5
  nonlinear route reaches the descriptor path before the basic/deaggregate
  path; its fragment-header helpers remain unchanged.
- The legacy nonlinear receive/clone contract is now enforced rather than
  assumed: no mixed linear head, at least one page fragment, and enough
  first-fragment bytes before the first MAP dereference. Computed current
  frame length must fit **that fragment** before next-header access or
  cloning. The check does not require `nr_frags == 1`, and does not require
  an entire multi-frame aggregate to fit the first fragment. The basic
  one-frame/deaggregated path separately requires that whole frame's extent
  to fit before its pointer is dereferenced.
- C short-circuit order excludes zero-fragment input before fragment-size
  access. Unsupported mixed-head/split-frame legacy layouts fail closed;
  data is not pulled, linearized or reinterpreted to force acceptance.
  This is an intentional safety boundary, not proof that every imaginable
  nonstandard SKB shape is a supported transport input.

The final module is:

`control-rmnet-core/rmnet_core/run07/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`

SHA256:
`7fa0bfcf5d79443156687c245ea3e39b995f39e1b21d1f28370fb1dac3d54191`.
The production build review reports compile/link/modpost PASS. Independent
read-only checks against this exact file passed:

- All 15 complete DWARF layout reports listed for run04 are byte-for-byte
  unchanged, including sizes/member/bitfield positions and the relevant
  common-kernel private-data base.
- All 25 callback KCFI checks match shipping stock exactly.
- Five core getter instruction/relocation reports match run04 exactly;
  stock accessor/allocation constraints remain satisfied.
- The queue import is still `system_wq`, with no `system_percpu_wq` import.

The independent [bounds review](coloros17-stage-a-binary-review.md) also
reviewed the added fragment guards. Its host/static model and mutation
controls are not kernel execution or a physical networking test. This
document does not label any CVE fixed or claim exhaustive RMNET security.

Normal RMNET/mobile data, tethering, VPN, USB-networking, aggregation and
teardown physical validation are **NOT TESTED**. In particular, exercise
normal and multi-frame traffic plus the fail-closed legacy-shape boundary
before approving deployment. Stock partitions and the released boot-only
artifact remain untouched; this driver fix would require deploying the
reviewed replacement module, not merely a new boot image.

Final component private/KCFI result: **PASS**. Final full-population oracle,
packaging, physical validation and release decisions are separate gates;
no automatic deployment is authorized by this review.

### Final full-population/provider policy result

The build review's final evidence at
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/hybrid-oracle-rmnet-run07/`
was inspected. `population-exactness.json` proves the only changed module
location is `vendor_dlkm/lib/modules/rmnet_core.ko`; no SDK/control provider
module is substituted, and every stock module-loading metadata file remains
unchanged. This is a staged module-population model, not a flashed image.

| Gate | Final static result |
|---|---|
| Module locations | 1,070 |
| Active compatible / dormant / blockers | 1,032 / 38 / 0 |
| Undefined ELF / hard required / weak | 60,707 / 60,701 / 6 |
| Hard imports resolved | 60,701 / 60,701 |
| CRC records matched / mismatched | 61,777 / 0 |
| Missing providers / namespace failures | 0 / 0 |
| Vmlinux-first edges / provider mismatches | 54,785 / 0 |
| Compiled unsigned-import edges / policy failures | 54,986 / 0 |
| Protected-export collisions | 0 |
| Unchanged ambiguous provider records | 3,220 |
| RMNET retained consumer edges with one actual provider | 56 |

The decoded frozen-kernel policy contains 10,381 permitted imports and 633
protected exports. Its signature result retains the frozen signing
classification; the policy check is **not** a new cryptographic signature
audit or AVB image verification. No partition or runtime module was changed.

Final source provenance is the isolated sequence:

1. `4ee253465786329a1156de7aad9000e9452a841c` — stock workqueue provider
   compatibility, with real compiled-policy rationale.
2. `084e43e044d572f30bec18ac3f0523d57d1c1351` — individual authoritative
   Qualcomm `ea47e85e...` fix, preserving upstream provenance.
3. `b7c709d8cd1381837afd97f170af6c5b3190a078` — separately reviewed local
   count/short-header/fragment-shape residual guards.

**Final static private/KCFI/provider/load-policy qualification: PASS.**
Common-kernel implementation/configuration, the released boot-only artifact and
the stock on-device partitions are unchanged. Physical qualification,
firmware/AVB packaging and deployment approval remain outstanding.

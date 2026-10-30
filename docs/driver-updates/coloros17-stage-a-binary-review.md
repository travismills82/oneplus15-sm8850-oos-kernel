# ColorOS 17.0.0.102 Stage-A shipping-binary and bounds review

Review date: 2026-10-30. Scope: read-only instruction/relocation inspection of
the exact shipping modules, followed by independent review of the proposed
RMNET source patch. No driver implementation was changed by this review. No
malformed traffic, exploit, module loading, device operation or physical test
was performed. A source patch is not a deployed security fix.

## Inputs, identity and source confidence

The inputs are the original extracted ColorOS 17.0.0.102 `vendor_dlkm` files,
also identified in the [complete module inventory](coloros17-module-inventory.md).
They are not binaries rebuilt from the public source checkout.

| Input | Partition-relative path | SHA256 | GNU build ID |
|---|---|---|---|
| CNSS2 | `vendor_dlkm/lib/modules/cnss2.ko` | `15a4c8e23dd3c5f588dacd3aee1a28959a8eb7b047fa656d6da26dc215b91d8c` | `5fd0ef87ab8780303872c9b489f8832c0869fc85` |
| RMNET | `vendor_dlkm/lib/modules/rmnet_core.ko` | `220a9dadafa4250ead10b78a9195399aacbe11934ff9558ffd142b45157ed8c2` | `e54fe2bebc819fcc428973f0a811b992ddca0241` |

Both are unsigned AArch64 ELF modules without BTF or DWARF. Their exact source
commit/configuration is **UNKNOWN**: no matching ColorOS `.102` source archive
or manifest has been supplied. Matching symbol CRCs, nearby public source,
version strings and matching function names do not establish exact source
identity. This review therefore separates direct binary observations from
source-level patch review and from unproven runtime reachability.

The reviewed public source starts at
`efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c` on the isolated
`experiment/coloros17-sm8850-dlkm-updates` branch. The relevant RMNET import is
`fdd9ce8fed3f3476f67c144bb5a44c6a60b101c7`; its `.102` label alone does not
prove that it produced the shipping module.

### Original Qualcomm references

| Component | Official repository | Reviewed commit | Subject | Cached patch SHA256 |
|---|---|---|---|---|
| CNSS2 | [wlan/platform](https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/wlan/platform) | `ed87beed64bafbbfdc2f6f087132753c7fa00077` | `cnss2: fix gpio_free crash on probe failure path` | `089edd1afffedc64ae8a1eff1e87f455fc7fe14c72d92d5bad29ca164ae92b08` |
| RMNET | [datarmnet](https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/datarmnet) | `ea47e85e97b72674a30e5846975c5a1eb04c6fde` | `datarmnet: Fix OOB read and length underflow in MAP coalesced packet ingress` | `704b2ff8abaa63f8edc1fab2aef8b8cc63d21e8de8cc7f8669165a963057346c` |

The separate [source/provenance comparison](coloros17-upstream-comparison.md)
records release refs and prerequisites. No CVE identity or device exploitation
claim is inferred from either patch title.

## Binary extraction method

Ubuntu LLVM `llvm-objdump`/`llvm-readelf` 21.1.8 were used. Complete ELF symbol
address ranges and call relocations, not function-size similarity, underpin
the observations. Addresses below are relative to these exact ELF inputs.
The end addresses are exclusive.

| Module | Function | Start | Size, bytes | End |
|---|---|---|---|---|
| CNSS2 | `cnss_init_direct_cx_host_sol_gpio` | `0x7f84` | 400 | `0x8114` |
| CNSS2 | probe implementation | `0xe3c4` | 9,656 | `0x1097c` |
| CNSS2 | TSF handler registration | `0xc964` | 800 | `0xcc84` |
| CNSS2 | TSF handler unregistration | `0xcc88` | 204 | `0xcd54` |
| RMNET | `__rmnet_map_ingress_handler` | `0x90e8` | 1,028 | `0x94ec` |
| RMNET | `rmnet_map_process_next_hdr_packet` | `0xb12c` | 2,940 | `0xbca8` |
| RMNET | `__rmnet_map_segment_coal_skb` | `0xcedc` | 1,324 | `0xd408` |
| RMNET | `rmnet_frag_process_next_hdr_packet` | `0x3268` | 3,956 | `0x41dc` |

The first symbol-name-only RMNET extraction stopped at intermediate mapping
symbols; it is **not** evidence of absence. Complete range extraction replaced
it for the conclusions below. The linear/nonlinear segmentation wrappers are
inlined into their next-header functions; missing standalone wrapper symbols
are not missing functionality.

Raw disassembly and immutable-input manifests remain local under
`out/coloros17-driver-audit-reference/binary-review/`, outside source/release
payloads. The evidence manifest SHA256 is
`122db44da03fe91d1df97934dabb4a2bc73c11bbe16b369c104715544e69dee5`.
It contains no requirement to distribute proprietary module binaries.

## CNSS2 instruction evidence

### Direct-CX and SOL ownership state

In `cnss_init_direct_cx_host_sol_gpio`:

- `0x7fb0` calls `of_get_named_gpio`; `0x7fb4` stores the result at
  platform-data offset `0x1f7c`.
- `0x8024` calls `gpio_request` for that GPIO.
- The nonzero-result branch through `0x802c`, logging and `0x80c0` reaches
  the epilogue at `0x80fc` without storing `-EINVAL` to that GPIO field.

The request-failure invalidation added by `ed87beed64ba` is therefore absent
from that shipping function. Probe also contains the device-SOL lookup/store
at `0x100a8`/`0x100ac`, request at `0x10114`, and host-SOL request at
`0x10558`; the inspected failure paths log and exit without invalidating the
corresponding stored GPIO values.

### TSF cleanup

TSF registration requests its GPIO at `0xc9ac`. GPIO-to-IRQ failure frees it
at `0xcbc8`; IRQ-request failure frees it at `0xcba4`. Neither inspected path
resets the freed GPIO field. Unregistration frees the GPIO at `0xccc0` and
resets the IRQ field at `0xccd0`, but not the GPIO field. These are concrete
post-free-state differences addressed by the Qualcomm patch.

Probe does initialize the TSF GPIO/IRQ pair to `-EINVAL` at
`0xef98`–`0xefb0`. This is **not** proof that all three SOL fields have the
same sentinel initialization. Whole-callgraph SOL initialization in the
shipping binary remains **UNRESOLVED**.

The official patch parent initializes three SOL fields to `-EINVAL` on probe
entry; the current public source lacks that prerequisite. Applying only the
forward-applicable error-path stores is not a complete ownership backport.
Selection requires coherent initialization/free/unwind/retry review, not a
partial patch presented as a fixed lifecycle. Runtime GPIO failure exposure
and device exploitability were not established by this inspection.

## Shipping RMNET instruction evidence

In `__rmnet_map_ingress_handler`, the QMAP byte read at `0x9184` precedes a
minimum-four-byte guard in that function. The packet length loaded at
`0x9198` is byte-swapped at `0x91ac`–`0x91b0`; padding is masked at
`0x91b4` and subtracted at `0x91c0` without an intervening rejection of
`pad > pkt_len`. Later header-pull checks at `0x923c`–`0x92fc` do not
establish the new complete packet/padding/checksum-trailer bound.

The linear coalescing path in `rmnet_map_process_next_hdr_packet` reads IP
version at `0xb6ec`, IPv4 IHL/protocol at `0xb710`–`0xb724`, and TCP doff
at `0xb7ec` without the new minimum/dynamic-header checks at those points.
The NLO packet length loaded at `0xb98c` is used in the header subtraction
at `0xb998`, stored as `u16` at `0xb99c`, and can be delivered at `0xb9c8`
without a separate whole-frame cumulative-payload validation prepass. The
GRO fast path likewise performs the subtraction at `0xb8c4`–`0xb8d8`.

The nonlinear coalescing implementation in
`rmnet_frag_process_next_hdr_packet` has the same specific absence: its
single-NLO fast path loads length at `0x3eb8`, subtracts headers at `0x3ee0`
and stores GSO size at `0x3ee4`; the normal loop loads length at `0x3fe4`,
subtracts at `0x3ff0`, stores at `0x3ff4` and can deliver at `0x4024`.
Nearby NLO-index/array checks are not packet-length or cumulative-data bounds.

**Conclusion limited to inspected instructions:** specific guards introduced
by `ea47e85e97b7` are absent from these shipping paths. This does not prove
the exact public source revision, all caller invariants, a reachable malformed
modem/offload input path, a runtime incident or exploitability. Those remain
**UNRESOLVED** until matching-source/build/runtime evidence exists.

## Independent RMNET source-patch review

This section reviews the complete cached patch and its current public-source
callers. It is not a shipping-binary source-match assertion. A non-mutating
`git apply --check --directory=vendor/qcom/opensource/datarmnet` succeeded.

The original patch changes only three implementation files:

```text
core/rmnet_descriptor.c
core/rmnet_handlers.c
core/rmnet_map_data.c
```

It changes no public/private structure declaration, exported prototype,
namespace, export class or wire-format definition. New bookkeeping is local
stack state. These facts reduce ABI risk but do not replace compilation,
actual export/CRC checks, private-type review or a matched-provider oracle.

### Correctly addressed bounds

The patch adds minimum MAP-header/padding checks; checks base and variable
IPv4/TCP header lengths; validates IPv6/UDP header availability; prevents
nonempty NLO packet lengths from being no larger than shared headers; and
checks accumulated payload before normal segmentation queues any packet.
The nonlinear path obtains/copies headers with checked fragment helpers.
`rmnet_map_ingress_handler` sends nonlinear coalescing/MAP-v5 traffic to the
fragment path; the ordinary coalescing SKB branch must be assessed in that
caller context rather than assuming arbitrary partial linear headers.

The cumulative byte accumulator is `u32`. The representable bound from six
NLOs, a 16-bit length and an 8-bit count is below 101 million bytes plus small
headers, so no 32-bit arithmetic overflow was found in the new formula.
Packet copying advances cumulative `data_offset`; it does not incorrectly
derive an offset from the current NLO length times the global packet index.

`rmnet_map_v5_csum_buggy()` requires exactly one NLO containing exactly one
packet. Therefore zero-count input cannot take that workaround. The nonlinear
workaround derives GSO size from NLO length and gains explicit bounds in this
patch. The linear workaround reuses the existing SKB without the NLO-length
payload/GSO derivation; its early return is not a demonstrated length-underflow
defect. The new whole-NLO prepass applies to normal segmentation/GRO paths,
not literally every early-return path.

### Residual issues: an unchanged upstream patch is not a complete result

1. **Aggregate packet-count wrap:** both existing coalesced-header validators
   retain an `u8 pkts` accumulator. A first count of 1 followed by 255 wraps
   the aggregate to 0 before comparison with `RMNET_MAP_V5_MAX_PACKETS=48`.
   `num_packets` is a full `u8` field, so the values are representable. The
   later byte-bounds prepass does not substitute for the intended count cap.
   A minimal local adaptation should widen the aggregate, not impose an
   undocumented eight-packets-per-NLO constraint.

2. **Nonlinear zero-count GRO:** the new nonlinear GRO condition still permits
   `num_nlos==1`, `csum_valid` and `num_packets==0`. Its length expression can
   pass and queue a descriptor with `gso_segs==0`. The linear patch explicitly
   guards this count; the nonlinear condition needs the matching nonzero
   guard. Normal unused/empty NLOs should remain ignored, not arbitrarily
   rejected. This is invalid-descriptor acceptance, not a reproduced crash.

3. **Earlier short-header paths require closure review:**
   `rmnet_map_deaggregate()` reads `maph->pkt_len` after only excluding a
   zero-length SKB, before the newly added four-byte ingress guard runs.
   Linear next-header dispatch reads the next-header type before verifying
   that header's actual byte availability; the coalesced-header validator
   reads fields before the new declared-`pkt_len` minimum test. A declared
   length is not itself proof of actual SKB storage. The nonlinear header
   helpers provide checked access, but the linear/direct/no-deaggregation
   path still needs an explicit actual-header-availability proof or a narrow
   adaptation. No complete malformed-SKB safety claim is made here.

These issues predate `ea47e85e97b7`; they are not regressions caused by its
new guards. They prevent labeling the unmodified patch a comprehensive MAP
hardening result. An independent follow-up, with separate local provenance,
is required before selection. No implementation adaptation is included in
this document.

### Proposed future non-device validation cases

Kernel/fragment fixture tests below are requirements, **NOT EXECUTED** kernel
results. A bounded host-model subset is recorded separately below; it does not
replace fixtures using real allocated SKBs/fragment descriptors. Do not send
malformed traffic to a device.

- Valid IPv4/IPv6 TCP/UDP, GRO on/off, checksum-bug workaround, and mixed NLO
  lengths: byte-identical expected payload/order and correct GSO metadata.
- Truncated main/next/coalescing headers at every boundary, with and without
  deaggregation, and linear/fragment-backed forms: reject before field access.
- IHL below the base header or beyond available bytes; TCP doff below the
  base header or beyond available bytes; truncated UDP and IPv6 extensions.
- NLO count 0/1/6/7; packet aggregate 48/49 and representable wrap case;
  zero-count single-NLO GRO; empty unused NLO entries remain harmless.
- NLO `pkt_len` below/equal/above shared header size; cumulative payload
  exactly fitting and one byte over; later invalid NLO queues no earlier
  segmented output; checksum-workaround length errors reject safely.
- Valid mixed-length NLOs advance offsets correctly; fragment boundaries,
  allocation failures and cleanup leave no leaks, double-free or stale list
  entry. Sanitized kernel/KASAN/UBSAN fixtures are still required.

### Minimum proposed follow-up contexts

The original Qualcomm patch remains individually attributable. A separate local
adaptation should contain only independently reviewed residual guards:

| Context after the original patch | Minimum proposal | Reason / limit |
|---|---|---|
| Both `rmnet_map_data_check_coal_header` and `rmnet_frag_data_check_coal_header` | Keep `u8 veid` and per-NLO `u8 pkt`; widen only local aggregate `pkts` to `unsigned int` | Six full-byte counts fit naturally; reject aggregate greater than 48 without wrapping. Do not invent a per-NLO limit of eight. |
| `rmnet_frag_segment_coal_data` single-NLO GRO condition | Require nonzero `coal_hdr.nl_pairs[0].num_packets`, matching the linear condition | An empty NLO falls through without queuing a zero-segment descriptor; ordinary empty/unused NLO handling remains unchanged. |
| `rmnet_map_deaggregate`, before reading the main header | Replace the zero-only test with a `sizeof(*maph)` minimum; reject nonlinear mixed-head/missing-frag/short-frag0 layouts before the data pointer | The downstream ingress guard runs too late for this parser. The legacy clone implementation assumes zero linear head and a frame contiguous in frag0. |
| `rmnet_map_deaggregate`, after calculating packet length but before next-header reads/clone | Require the current packet length to fit current frag0 for nonlinear input | Permit multiple fragments/frames per aggregate when each current frame fits; do not require the aggregate to fit frag0 or `nr_frags==1`. |
| `__rmnet_map_ingress_handler`, before reading the main header | For nonlinear single-frame input, reject mixed head/missing frag/frag0 smaller than the complete frame | Enforce the existing all-in-frag0 data-pointer contract without moving head/fragment data. |
| `rmnet_map_process_next_hdr_packet`, before header-type dispatch | Require actual SKB bytes for main MAP plus checksum-header size | Protect next-header type/checksum fields before reading. Current MAP-v5 callers route nonlinear frames to the descriptor path. |
| `rmnet_map_data_check_coal_header`, before data/field access | Require actual SKB bytes for main MAP plus complete coalescing header | The declared packet length cannot establish actual presence of all NLO fields. |

Do not change the meaning of declared `pkt_len`: the existing deaggregation
code adds the MAP-v5 checksum header separately. These are actual-storage
minimum checks, not a new wire-format interpretation. The later official
packet-length checks and stable semantics must remain intact.

`pskb_may_pull()` may move data from fragments to the linear head and invalidate
cached pointers. It must not be added mechanically to deaggregation: that
function's nonlinear branch currently clones the complete packet from frag0,
which a partial head pull would change. `skb_header_pointer()` can inspect a
split header without mutating SKB storage; `rmnet_frag_header_ptr()` already
provides checked descriptor-header access. Any wider handling of nonlinear
deaggregation must explicitly preserve or validate the existing all-in-frag0
contract, including packet clone bounds. A total-byte check is not a universal
proof of contiguous storage. No new shared/exported helper or public type is
required for the narrow linear guards.

### Executed offline requirement models

The standalone
[host audit helper](../../tools/driver-updates/test_coloros17_rmnet_bounds_model.py)
is at `tools/driver-updates/test_coloros17_rmnet_bounds_model.py`. Run from the
repository root:

```sh
python3 -B tools/driver-updates/test_coloros17_rmnet_bounds_model.py -v
```

On 2026-10-30, **23 audit tests passed**, with no skipped tests: 14 host-model
tests and 9 separate static `SourceContracts` tests. The run printed the exact
SHA256 of its four source inputs. The scalar models cover logical
linear/split-header truncation boundaries, MAP padding/trailer inequalities,
the aggregate wrap counterexample, cumulative payload/underflow boundaries,
unused NLO preservation, nonlinear zero-count GRO and the legacy per-frame
frag0 fit requirement (without restricting aggregates to one fragment). They
specify acceptance requirements; they do **not** execute the driver's actual
SKB/fragment functions or prove that those functions enforce the models.

One test extracts the exact existing `rmnet_map_v5_csum_buggy()` implementation,
its enum definitions and wire declarations from the current source and compiles
them with the host C compiler. It checks host sizes of the copied declarations
and exhaustively evaluates 524,288 combinations of NLO count, packet count,
close type and close value. This confirms the reviewed one-packet workaround
predicate, not ARM64 module ABI, SKB behavior, memory safety or race freedom.
The temporary host executable is removed after the test. No generated source,
binary, network packet or device operation is retained or deployed.

The static `SourceContracts` extract real source function bodies with comments
and literals removed. They verify guard ordering before main/next/coalescing
header reads, both aggregate-counter widths, the nonlinear GRO nonzero-count
condition, and separate normal-path prevalidation before segmented delivery.
They also check the independently bounded nonlinear fast paths. The recognized
linear checksum-workaround early return is not mislabeled as occurring after
the normal prepass. Eleven in-memory negative controls remove individual guards,
narrow counters or remove prevalidation and require the static checks to fail;
driver files are never changed by these controls.

Static lexical contracts are regression alarms, not a formal control-flow
proof or execution of the driver. These results are **not** a module build,
a private-ABI proof, a kernel selftest, a KASAN test or physical qualification.
The residual three-file diff passed checkpatch with 0 errors/0 warnings across
93 lines; the complete original-plus-local three-file diff passed with
0 errors/0 warnings across 439 lines. These are style checks only. Subsequent
source changes require rerunning all checks; all runtime gates remain separate.

## Selection / deployment gate

CNSS2 remains prerequisite/ownership-review blocked. The scoped RMNET local
adaptation was present for the final read-only review: its actual guards passed
the static contracts above. The original patch alone remains insufficient for
that result. Matching-source/private-ABI evidence, production builds, the
provider oracle and separately authorized runtime qualification are still
required; these host/static results do not satisfy those deployment gates.

Neither module can be updated on the phone merely by editing its unused
source in a boot-only build. A matched-module experiment needs reviewed
provider precedence, signing/load policy, dependency closure, all relevant
stock runtime consumers, and a separate install/rollback plan. No replacement
binary, DLKM image, release claim or physical qualification is produced by
this review. Existing qualified artifacts and the boot-only release are
unchanged.

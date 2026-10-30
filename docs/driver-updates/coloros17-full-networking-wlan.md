# ColorOS 17 networking and WLAN integration ledger

Investigation: 2026-10-30. Target is only OnePlus 15 ColorOS
`CPH2745_17.0.0.102(EX01)` and the qualified 6.12.114/KMI6 common kernel.
There is no Lineage build or firmware dependency. The initial driver-fix
point is `e65991a0500c64fedacfc9a1e540c952d36c98e2`; subsequent source-safe
build/STA-mode adaptations are `566ab26631116` and `09decca5f42a2`. Common source,
configuration, signing/enforcement policy and device partitions are unchanged.

## RMNET: preserve completed series; do not partially port teardown

The existing queue-provider preservation (`4ee253465786329a1156de7aad9000e9452a841c`),
official ingress bounds (`084e43e044d572f30bec18ac3f0523d57d1c1351`, original
`ea47e85e97b72674a30e5846975c5a1eb04c6fde`) and coherent local packet/header
guards (`b7c709d8cd1381837afd97f170af6c5b3190a078`) remain intact.
The frozen run07 candidate retains 140 exports, 156 version records, 155
imports, all 56 retained consumer edges, 15 inspected private layouts and
25 mapped KCFI words. The independently repeated combined 1,070-module
oracle is recorded in the security/audio ledger. This does not imply a
radio, hardware DMA or suspend runtime test.

`c85841b2276e35341ee21502ecea6258d50dea74` fixes a real source-level
sleep-in-RCU concern in the PM notifier, but its newer teardown assumptions
do not match this source. A mechanical RCU-to-SRCU conversion is not applied:

- `qmi_rmnet_work_exit()` returns early without unpublishing when its
  powersave workqueue is absent; SRCU readers could outlive the allocation.
- Client deletion and setup failures independently free `qmi`.
- The WDA handle is released before the current work-exit grace period.
- The PM notifier uses the derived port after its current read-side unlock.
- Waiting for SRCU while holding RTNL requires proving the callback cannot
  reacquire RTNL through powersave operations.

The smallest correct next unit is a coherent publication, notifier-reader,
WDA/workqueue-resource and port-teardown lifetime repair, including every
free path and lock order. No descriptor/DMA/workqueue cleanup is weakened
to make the upstream patch fit. The ingress hardening does not claim to
resolve this separate PM lifetime issue.

A final complete visible caller pass strengthened that hold. The notifier
touches/cancels `rmnet_work` before entering the proposed SRCU reader section;
work-exit destroys that work independently. DELETE idx1 can free `qmi`
without work-exit; SETUP failure also frees it, and successful publication
does not require a powersave queue. Both DELETE0 and `qmi_exit()` release WDA
before the proposed grace period, while notifier paths can initialize/send/
wait for QMI transactions. Port teardown then frees the port after `qmi_exit`.
The final powersave notification traverses externally owned `ps_list`;
SHS/offload deregistration uses ordinary RCU, not SRCU, and an SHS callback
can block in `cancel_delayed_work_sync`. Global qmi/work publication is not
consistently tied to the same port when multiple real ports are admitted.
An unconditional pointer clear would not establish correct owner lifetime.

No actual RTNL inversion was found in the inspected PM leaf call chain;
RTNL remains a proof obligation, **not an observed or diagnosed deadlock**.
The coherent next unit must serialize the pre-reader work snapshot, every
qmi/WDA/port free, callback deregistration/grace and per-port ownership.
These are concrete source issues, not proof that the opaque shipping binary
has the identical implementation or that an experimental image is safe to deploy.

## IPA: do not confuse QMI validation with a permitted address contract

`0bfbd1e90f84f474a34ce8286893356af9d8348b` is held. Its new range checks
unconditionally restrict mappings to the IPA register aperture. This tree
passes both control and data mappings through the helper and explicitly
handles ordinary memory outside that aperture. The firmware/QMI contract
must establish which physical ranges are legitimate before this restriction
is accepted. Arithmetic-only partial changes would not establish that the
intended security fix is complete.

The unchanged IPA SDK build has seven USB-interface export CRC differences
versus shipping providers, independently of that patch. Its genuine generated
symbol table is a compilation input, not a permission to replace stock IPA.
No IPA module is substituted into the qualified hybrid.

## WLAN source fixes and exact provenance

Canonical official repositories (including the actual vendor namespace):

- [qcacld-3.0](https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/wlan/qcacld-3.0.git)
- [host-cmn](https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/wlan/qca-wifi-host-cmn.git)
- [platform](https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/wlan/platform.git)
- [fw-api](https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/wlan/fw-api.git)

The first five original fixes below are from the reviewed 10800 family;
the P2P fix is from `WLAN.LA.2.0.r3-10900-kaanapali.0`. Exact tag tips and
newer, different release trains remain in `coloros17-upstream-refs.tsv`.
No whole endpoint/directory is imported.

| Original commit | Local commit | Change and decision |
|---|---|---|
| `69a2230a0e2888797b5ad9afb898e7955f438a9a` | `1f55863ac696b` | Reassociation MLIE span uses the reassociation, not association, IE offset. |
| `f0eb2ec58ebbc3f5b1c595399023d674d435bc6f` | `0a7d4d80444d6` | Original HE index guard preserved individually, then corrected/extended below. |
| `7bf5821c503b536212ea8993bb854480a494e28c` | `b9dd611f14787` | Reject LL_STATS during MLO connection progress using existing get/put-aware helper and non-MLO stub. |
| `11a03259c48fa54f6bc67a39309740e62b776776` | `1f6dd0d1aae3d` | Completion paths acquire a vdev through the existing `vdev_lock`-protected helper and release the exact acquired object. |
| `b0af9d5ebef64de3178f49dcbddf54ce2b53e245` | `4fd9bfcf1136d` | Disabled INI runtime PM returns false before querying kernel PM state. |
| `77095667660c9eaeafd58083065722a208c2247e` | `e65991a0500c6` | Require complete three-byte P2P TLV header and include it in remaining-span validation. |

Local `1d91a86fac0d5` is deliberately separate: `MAX_HT_MCS_INDEX` is 7,
whereas the actual HE table has 14 entries (0..13), and the VHT table has
12 (0..11). Bound the real table index both before RSSI iteration and after
firmware-MCS fallback; preserve valid HE13/VHT11 instead of truncating them.
The reassociation helper rejects frames shorter than its ten-byte IE offset
before pointer arithmetic. No UAPI/private structure/configuration is changed.

Peer review found that the imported MLO progress check also reached SAP,
whose MLO context allocates `ap_ctx`, not the helper's dereferenced `sta_ctx`.
Local `09decca5f42a2` short-circuits the call for non-STA adapters. This
preserves the intended station-connect race guard without adding an AP NULL
path. The original upstream patch remains individually preserved. An actual
C-predicate test executes STA/SAP/P2P cases under ASan/UBSan; removing the
STA guard fails its negative control. No AP context is retyped.

### Actual build boundary, not a fabricated PASS

The stock runtime uses `qca_cld3_peach_v2`. A normal Canoe control captured
6,200 unchanged inputs across qcacld, host-cmn, platform and fw-api and used
the actual generated grep/config header. The compile reaches
`wlan_hdd_cfg80211.c:37035` and rejects `.set_monitor_channel`:

```c
/* WLAN control implementation */
int callback(struct wiphy *, struct cfg80211_chan_def *);
/* Current common cfg80211_ops declaration */
int callback(struct wiphy *, struct net_device *, struct cfg80211_chan_def *);
```

An incompatible cast or warning suppression is not a repair. Nor can an
unused argument simply be added and deployed against an unverified old
stock cfg80211 callback provider: its second argument would be interpreted
incorrectly. A proven stock-header contract or a fully matched cfg80211/WLAN
module closure is required. No WLAN candidate enters the hybrid package.
The source fixes remain isolated and host-tested rather than abandoned
because of the independent AVB blocker.

Local `566ab26631116` subsequently replaces the version-number guess with
two separately typed wrappers around the unchanged single-monitor helper.
The initializer uses C11 `_Generic` on the actual header field, without a
default: old headers select the old callback; new headers select the new one;
an unknown signature fails compilation. No callback cast, CFI relaxation or
type-string masking is used. The new-header candidate remains quarantined
from stock cfg80211 and cannot be deployed as a one-module replacement.

Stock ELF confirms that this is a real provider boundary: stock cfg80211's
`cfg80211_set_monitor_channel` reads `ops + 0x138`, passes wiphy in x0 and
chandef in x1, and checks KCFI `0x29b128f3`. Stock peach uses that same KCFI
and consumes x1 as chandef. Current common's three-argument API is introduced
by `d6aef302c4180`, original upstream
`9c4f830927750a2bf9fd9426a5257f0fdce3b662`. Both stock cfg80211 copies
are byte-identical; normal loading selects vendor_dlkm, while the vendor-boot
copy is dormant. Its 373 retained stock import edges span mac80211, kiwi,
peach, wcn7750 and wonder. A minimum cfg80211+peach prototype migration
therefore also needs review of every other actual cfg80211 consumer, not
only the callback that triggered the compiler error. Disassembly/JSON are
preserved under `build/wlan-provider-boundary/` in the evidence root.

Evidence:
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/build/wlan-peach-control/run01/attempt-generated02.log`.

The final `09decca5` matched-source candidate subsequently completed all838 C
compilations, aggregate linking, natural modpost and final `.ko` generation
using genuine complete CNSS/FW/IPC/SMEM/QMI/SSR producer tables. Final module
SHA256: `cd959ed843711ab9b8a4ac7f431f7f920df7bd2db355f9b8861f6f303c9366b0`.
The complete frozen source proof and2,709 actual compiler dependency hashes
have zero unresolved/unexplained inputs. Earlier missing-SDK failures remain
recorded; none was ignored. The final emitted callback has natural KCFI
`0x7e862964`, not stock `0x29b128f3`. This is **compile PASS, stock runtime
boundary FAIL/quarantined**, not a flashable one-module WLAN update. No SDK
producer is silently installed to make a stock-consumer oracle pass.

Final actual-stock import audit (not a compatible-SDK substitution):

| Actual import | Stock provider CRC | Candidate required CRC | Result |
|---|---|---|---|
| `cfg80211_new_sta` | `0x83d71e16` | `0xe494dd96` | MISMATCH |
| `cfg80211_del_sta_sinfo` | `0x83d71e16` | `0xe494dd96` | MISMATCH |
| `wiphy_new_nm` | `0x7640d963` | `0xc67e1f4d` | MISMATCH |

Candidate has601 ELF imports/602 version records versus stock599/600; the
new valid imports `__hrtimer_get_remaining` and `ip_send_check` resolve.
There are zero exports and zero retained consumers of WLAN exports in this
population. `system_percpu_wq` independently fails unsigned-vendor import
policy. No protected-export collision is found. These real CRC/policy failures
and the callback KCFI change prove the quarantine decision; do not force-load,
alter CRCs, widen permitted imports or silently supply a new cfg80211 provider.

### Host verification

`python3 tools/driver-updates/test_coloros17_wlan_bounds.py`: eight tests PASS.
The actual HE/VHT functions and source tables execute under ASan/UBSan across
573,440 scalar combinations. The actual P2P TLV function executes across
250 remaining sizes and all 65,536 attribute lengths (16,384,000 calls);
a deliberately removed span guard is detected by ASan. OUI lookup is an
explicit host shim and is not claimed tested. Static contracts cover the
MLO precheck, exact completion get/put calls and disabled-PM branch.
These tests do not prove firmware, RF, object-manager concurrency or module
compatibility, and do not turn the failed full-module control into PASS.

## Other deferred networking candidates

`ed87beed64bafbbfdc2f6f087132753c7fa00077` requires the complete SOL GPIO
initialization/free/ownership prerequisites, not only new request-error
assignments. `b85c8ae36f9a0e2f995b0eddf4b546d849ba6fbb` needs firmware
loading and runtime/system PM transition proof. `2329b887...` changes a
legacy defragmentation/private layout whose active SM8850 path is not proven.
`bf46d278...` has unresolved DMA-address versus physical-PFN mmap semantics.
Later datapath/passthrough/BDF-name paths absent from this generation remain
inapplicable to those exact fixes; this is not blanket security clearance.
Bluetooth, stock RFKILL/HIDP/PPP, IPA offload and vendor hardware modules
remain untouched. No idle-watchdog warning is claimed fixed by these patches.

## Provenance trailer clarification

Five new commit messages contain noncanonical repository path spellings:
`b9dd611f14787`, `1f6dd0d1aae3d`, `4fd9bfcf1136d` should use the
`qcom-opensource/wlan/` URLs above, not `qcom/opensource/wlan/`.
`e3e7b8821eabb` comes from official `platform/vendor/opensource/camera-kernel.git`;
`cda06aea6ef78` comes from official
`platform/vendor/qcom/opensource/audio-kernel-ar.git`.
Their exact original SHA, authorship, messages and relative patch IDs were
verified; this clarifies repository URLs without rewriting their commits.

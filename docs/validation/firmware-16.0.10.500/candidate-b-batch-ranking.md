# Candidate B project-delta ranking

## Effective comparison boundary

Candidate A runtime source head `99077ba8e1792b46d341d60f446b79d260f5f639`
was compared with the pre-ACK physically qualified controlled kernel tree at
`36f40a44d700422969dc7debb6519c0f9ab977d0`. The comparison uses effective
runtime/build inputs, not commit-name equivalence.

Within `kernel_platform/common`, the trees differ at 107 paths: 98 modified,
four added, four deleted, and one certificate rename. The complete source-path
inventory is already recorded in `ack-old-base-to-oos1610500-r53.tsv`; the
coherent runtime and build-contract classification is in
`candidate-b-project-delta.tsv`.

The former controlled tree has no effective change under `fs/crypto`,
`block/blk-crypto*`, `drivers/md/dm-default-key*`, `drivers/md/dm-crypt*`,
`security/keys`, ext4, UFS, SCSI, or Qualcomm ICE paths. It does contain active
F2FS, dm-bow, EROFS, and broad legacy memory/IOMMU changes. Those are isolated
from the first Candidate-B batch.

## Stock-DLKM boundary

The native OOS 16.0.10.500 system policy remains authoritative:

- 103 system modules;
- 82 normal `modules.load` entries;
- `wwan.ko` at entry 28; and
- exact system_dlkm SHA-256
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`.

All former conversions of Bluetooth, NFC, TLS, USBMON, IKHEADERS, USB Ethernet,
or other requested system modules into Image are deferred. Reintroducing those
conversions while retaining the native load list could cause duplicate or
stale load requests and would defeat one-variable isolation.

The exact stock vendor_dlkm SHA-256
`157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
also remains immutable. External WLAN, cellular, audio, camera, graphics,
Bluetooth, NFC, and CoreSight source upgrades therefore have no place in a
Candidate-B boot-only batch.

## Ranked bounded runtime candidates

| Rank | Batch | Change | Risk | FBE interaction | DLKM interaction |
|---:|---|---|---|---|---|
| 1 | B01 | Binder transaction target lifetime pinning | low-medium | none; no storage path | none; Binder is built into Image |
| 2 | B02 | BPF per-CPU map copy bounds fix | low-medium | none | none |
| 3 | B03 | Eventpoll RCU lifetime series | medium | core VFS event path, not encryption | none |
| 4 | B04 | TCP BBR default and CAKE/PIE/HL config | medium | none | no retained provider change expected |
| 5 | B05 | Power-efficient workqueue default | medium | none direct; global scheduling behavior | none |
| 6 | B06 | AF_UNIX GC and SCC hardening | medium | none | none |
| 7 | B07 | AF_PACKET fanout lifetime fix | medium | none | none |
| 8 | B08 | IPv6 MLD skb lifetime fix | medium | none | shared WLAN/cellular networking runtime |
| 9 | B09 | SFQ validation/GSO fixes | medium | none | networking stress required |
| 10 | B10 | SKB shared-fragment propagation series | medium-high | none | foundational WLAN/cellular data-path semantics |

Storage/FBE, legacy OnePlus common overrides, pKVM/IOMMU, built-in conversions,
and all vendor-module source changes are explicitly deferred.

## Recommended Candidate B01

Select only commit `5c4058a84655b76abb22cf255dea5761788674d3`, which is a
coherent backport of ACK commits `1ac5be05b2854ba2329c0e52a67edd80b3ab4352`
and `2df1e89cb5aab41eee1ca77f03785d7a4809ad60` (upstream
`114a116aaa5f0295376cdf12da743c5bce3b20ce` and
`f223d27a546c1e1f48d38fd67760e78f068fe8c4`).

The r53 Binder implementation is demonstrably pre-fix. The patch changes only
`drivers/android/binder.c`: `binder_free_transaction()` takes `t->lock`,
captures `to_proc` and `to_thread`, temporarily pins the target thread, uses
the target process, and drops the temporary reference afterward.

This is a genuine memory-lifetime security fix in an always-built Android
kernel component. It changes no configuration, public export, module provider,
FBE/fscrypt/storage path, or device tree. It is independently revertible and
requires no system_dlkm or vendor_dlkm change.

Candidate-specific physical coverage should add Android framework/app-launch
stress to the standard boot, encrypted-user0, WLAN, cellular, Bluetooth, NFC,
deep-idle, and error-scan gates.

# Controlled-v1 TEST3 canonical baseline — 2026-08-21

## Scope

This document freezes the physically validated controlled-v1 TEST3 delivery
contract. It does not change kernel, WLAN, cellular, vendor-boot, or VBMeta
functionality.

The canonical release-candidate branch is
`feature/controlled-v1-test3`. Its kernel build lineage remains
`090459863b8ccf45599c6506461d144ab736d9a5`; later packaging, validation, and
documentation commits do not replace that source identity.

## Immutable TEST3 payloads

| Payload | Policy | SHA-256 |
| --- | --- | --- |
| `boot.img` | controlled-v1 | `25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313` |
| `system_dlkm.img` | corrected controlled-v1 load policy | `edebc94818e6fa4e214d58fd82fe46f6c513fc9850b3e7b77caf076a12270f05` |
| `vendor_dlkm.img` | Candidate A | `24e66015a3e4ea3583f895d529008d8c7c3706d7bc506ef550df936935127b80` |
| `vendor_boot.img` | exact stock, unchanged and not required in TEST3 package | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` |
| VBMeta | stock, unchanged and not packaged | n/a |

The sanitized non-flashing package is produced at
`out/controlled-v1-test3/` through:

```shell
tools/package-controlled-oos-stack.sh --profile controlled-v1-test3 \
  --boot <validated-boot.img> \
  --system-dlkm <corrected-system_dlkm.img> \
  --vendor-dlkm <candidate-a-vendor_dlkm.img> \
  --kernel-build-dir <kernel_aarch64-output> \
  --system-stage-dir <corrected-system-stage-dir> \
  --vendor-stage-dir <candidate-a-vendor-stage-dir> \
  --out-dir out/controlled-v1-test3
```

The packager refuses a payload whose hash differs from the physical TEST3
input. It excludes private signing material, device backups, live captures,
and temporary paths.

## system-DLKM load contract

`modules.load` is part of the controlled runtime ABI and delivery contract.
The accepted system image contains 46 modules and 46 ordered load entries.
`wwan.ko` is entry 21 and is required.

`tools/validate-system-dlkm-load-contract.py` fails when:

- `modules.load` is absent or empty;
- a required entry such as `wwan.ko` is absent;
- a listed module payload is missing;
- a built-in module remains as a stale load request;
- `modules.dep` lacks a consumer row or names a missing provider;
- not every delivered system module is covered by the reviewed load policy;
- Candidate A lacks `cfg80211.ko` or Peach-v2;
- cfg80211 is not ordered before Peach-v2; or
- Peach-v2's dependency row no longer names cfg80211.

The generated evidence is `system-dlkm-load-contract.tsv`. The current result
is 46 system entries, `wwan=yes`, stale entries 0, missing entries 0, and a
valid vendor cfg80211-to-Peach handoff.

## Vendor-DLKM boundary

Candidate A contains all 436 required vendor modules:

- 430 exact stock binaries;
- 3 controlled source replacements;
- 3 exact-stock payloads re-signed for the controlled trust boundary; and
- all 27 vendor-side IPA/GSI/RMNET/data closure modules byte-identical to
  OxygenOS 16.0.9.400(EX01) stock.

`vendor-dlkm-module-contract.tsv` records every module and action. This task
does not source-replace the accepted stock cellular closure.

## Physical bisection record

| Test | Result |
| --- | --- |
| TEST0 — r7 Image plus stock system/vendor DLKM | PASS |
| TEST1 — controlled-v1 Image plus stock system/vendor DLKM | PASS, including cellular |
| TEST2 — controlled-v1 Image plus corrected controlled system-DLKM and stock vendor-DLKM | PASS, including `wwan` and cellular |
| TEST3 — TEST2 plus Candidate A vendor-DLKM | PASS, controlled WLAN plus stock cellular simultaneously |

The first proven defect in the rejected controlled delivery was an empty
system-DLKM `modules.load`, which omitted `wwan.ko` from runtime. The bisection
does **not** identify IPA or RMNET as the root cause. The broader replaced
cellular set remains unvalidated and is outside this baseline.

TEST3 physically passed two boots, Visible LTE HOME, RMNET IPv4/IPv6 and
default routes, IP and DNS reachability, Wi-Fi 5/6 GHz WPA3, Wi-Fi reload, and
simultaneous WLAN/RMNET coexistence. Targeted runtime scans found zero relevant
unknown-symbol, CRC, vermagic, signature, or protected-export failures.

## Recovery installer compatibility

TWRP `feature/controlled-kernel-installer` at hardened commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` accepts independent `boot`,
`system_dlkm`, and `vendor_dlkm` payloads. `vendor_boot` is optional. It keeps
the dependency-first order `vendor_dlkm`, `system_dlkm`, optional
`vendor_boot`, and `boot` last; verifies backups and read-backs; refuses active
snapshot state; and rejects a data-media backup destination until user 0 is
decrypted.

Recovery was rebuilt from that exact commit. The resulting
`recovery.img` SHA-256 is
`1eb66b416b92e9a333ffe59e694cd18e770570dc624bd453011a6126023905af`;
its staged helper is byte-identical to the feature-branch source and its
embedded AVB footer verifies successfully. Live dry-run evidence remains a
separate device-side gate and performs no writes.

## A/B safety status

At TEST3 validation time slot `_b` was the controlled baseline and slot `_a`
was marked unbootable. Slot A must not be described as a fallback until its
contents have been inventoried, restored from verified production payloads if
needed, and physically boot-tested. TWRP and verified backups remain the
recovery path until that audit is complete.

```text
PASS — CONTROLLED WLAN + STOCK CELLULAR BASELINE ESTABLISHED
```

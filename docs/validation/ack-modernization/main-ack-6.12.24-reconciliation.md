# Main / ACK 6.12.24 history reconciliation

Date: 2026-08-24

## Purpose

This one-time merge reconciles the existing `main` release-engineering history
with the physically qualified ACK 6.12.24 runtime lineage without rewriting
either history.

- Previous main: `b0ee2ef425fbcc0873dafa4a49fc90a3ebb6a1a0`
- Qualified runtime HEAD: `4ac3490a2b57e951c27648aa63083a1f27d01f42`
- Merge base: `39bfc1db04fe323c1a6c75ee089d8eb4817002d4`
- Qualified tag: `ack-6.12.24-qualified`
- Safety branch: `main-pre-ack-6.12.24-reconcile`

The qualified parent is authoritative for every runtime or build-producing
path. The only permitted tree differences are release verification,
historical release records, documentation organization, and this
reconciliation record.

## Main-only commit classification

| Commit | Classification | Reason |
| --- | --- | --- |
| `c925d831ad04472877f8012ca3650244fe79d751` | A | Controlled vendor_boot integration is already functionally present in the shared qualified base. |
| `323e7011fc4aaffc4a7a2b0c81da2589dca747db` | A | WLAN .053 integration is already functionally present in the shared qualified base. |
| `8906fd47be43616ee8ed532ae571ecbe30dced49` | C | Bluetooth .046 runtime source import is represented by the later physically qualified tree. |
| `2d29402ace4666e65f9f5c22e1fca9778f060791` | B | Bluetooth closure audit documentation only. |
| `941d0eb41407680d4fbe4b1b81a8c4e92f1764e2` | C | External-DDK kernel-contract implementation is represented by the later qualified tree. |
| `b33a3c080dc4b1723c0968e2fd23ae1e142baeed` | C | DDK contract build ordering is represented by the later qualified tree. |
| `9b1f73526064ea1c8e8013db6f3b50ca63ca07a6` | C | Kernel metadata/functional-contract separation is represented by the later qualified tree. |
| `fe4e8a9b4acea1c9e70b956700ec4d1c8a17c827` | C | Canonical DDK output selection is represented by the later qualified tree. |
| `344da2da77c8762046eb04f7b64a5568c7d9ae5a` | C | Controlled-v1 DDK signing is represented by the later qualified tree. |
| `d5b13ee3f09b3912bd0c0d601c44b43fb07a7820` | C | Controlled signing algorithm selection is represented by the later qualified tree. |
| `7ef14dede8aab79bd41a1e83235d298064aefe26` | C | DDK import validation is represented by the later qualified tree. |
| `04e77c95c87f1cce4ecbdffd156fb39bc1cd91fd` | C | Bluetooth DDK packaging identity is represented by the later qualified tree. |
| `79c231fcd767d5dd73acbf04bb47152f4b6c31c6` | B | External-DDK release-contract documentation only. |
| `c6a0d74e998227ebda3b81e736d7669cdaa11820` | B | Bluetooth .046 physical-validation documentation only. |
| `04eb8d09ed71f4489dc1b726f96d1ba790549c3e` | A | Bluetooth .046 integration is already functionally present in the shared qualified base. |
| `96108e4068bc07a99c7c02cb162499f727f726ee` | A | Cellular/NFC integration is already functionally present in the shared qualified base. |
| `70e3b607ac1cb8ded9ccd84f738d6d6e47a664e8` | A | Subsystem modernization and tested packaging integration are already functionally present in the shared qualified base. |
| `735ef80fdf814151799ac9245fa6c3b850cc3921` | B | Live single-ZIP TWRP validation documentation only. |
| `d0f4d53c341f7e808ad332eceef0d0ee32076bc8` | B | r8 release engineering, manifests, notes, and verification tooling only relative to the shared base. |
| `f4d19ed81c5337aa9a1ea0c09622a6e148b92a32` | B | Release-verification CI only. |
| `b0ee2ef425fbcc0873dafa4a49fc90a3ebb6a1a0` | B | Documentation organization only. |

Totals: A = 5, B = 7, C = 9, D = 0, E = 0.

No runtime change missing from the qualified tree was found. Relative to the
shared base, current main changes only release documentation, release
verification tooling, its CI workflow, and documentation locations.

## Runtime contract

The reconciliation does not rebuild or replace any qualified payload. It
continues to reference the physically tested artifacts:

- boot: `3ceb46491d029586af1a6dc494b5baf4ddb973ad0c065c0960e4ed307d9d40b9`
- system_dlkm: `1739941b5bda37b52657a06340411c93fa6fff2a32ee1cfe5d32c28902d8846a`
- vendor_dlkm: `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`

The existing physical qualification remains authoritative:

- kernel: `6.12.24-android16-5-o-ga7f2fd6d686f-4k`
- modules.load entries: 46
- `wwan.ko`: entry 21
- ABI: PASS / empty
- KMI: PASS
- module and kernel error scan: PASS

## Main semantics

After this merge, `main` means the latest physically qualified runtime
baseline plus non-runtime release and documentation history. A metadata-only
commit may therefore be newer than the runtime qualification commit, provided
the recorded `runtime_input_delta` is zero.

- `runtime_qualified_head`: `4ac3490a2b57e951c27648aa63083a1f27d01f42`
- `main_metadata_head`: the reconciliation merge commit
- `runtime_input_delta`: 0

Future ACK experiments must branch from the reconciled `origin/main`.

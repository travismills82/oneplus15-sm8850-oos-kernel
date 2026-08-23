# Cellular Batch 01 static validation

Date: 2026-08-22

Branch: `experiment/cellular-batch-01`

Frozen base: `089b9d30c9c8837a2bbb9b721d08d66925b17a21`

Audit commit: `dd642735bfe36`

Device writes: none

## Scope

Batch 01 replaces exactly `rmnet_sch.ko` with a controlled-v1-signed build of
the current datarmnet-ext `.097` source. It does not import `.102` code. The
source identity is `234073f08cd430577ba69a7eaba5118c8991d41b`; the reviewed
runtime source `rmnet_sch_main.c` has SHA-256
`eb7d7558fee8f2e17bb425ca51f5715835a1c61d075d8f6b1484026bd6c6311e`.

All other 435 vendor modules remain byte-identical to the physically qualified
combined core baseline. In particular, WLAN053, BT046, NFC102, and the other
26 stock cellular modules are unchanged. Boot, system-DLKM, vendor boot, and
VBMeta are not candidate payloads.

## Kernel contract

| Gate | Result |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| `.config` SHA-256 | `b53d48b303059adb49a8dbe457145a4b7523a77fae621ea8d9e7b0b727e1615b` |
| Canoe generated config SHA-256 | `b5d038e4e03dde1664b036b2d7f2d6319d8089ce5f6987010a968fcefd1c7925` |
| `Module.symvers` SHA-256 | `de57709f3de38afb3e266481da09433687979ffb88ee607bda93ac4732dd7e0b` |
| Image functional differences | 0 |
| vmlinux functional differences | 0 |
| ABI diff | PASS, empty |
| KMI symbol checks | PASS |
| signing/trust identity | controlled-v1, unchanged |

The clean build used 101,648 declared common `KernelBuild` action inputs. The
release-contract architecture 2 guard passed before the external DDK target
was built. The only tolerated Image/vmlinux byte differences were the already
declared build-ID/DWARF metadata ranges; functional differences were zero.

## Module contract

| Property | Result |
| --- | --- |
| output | `out/cellular-batch-01/modules/rmnet_sch.ko` |
| SHA-256 | `4fdb7d1122e430731594c7eea9dc2e8686cd7c2668e05d39ef66bfa18b0c75b4` |
| bytes | 20,779 |
| vermagic | exact g6744 release contract |
| signer | `OnePlus 15 Controlled OOS Module Signing v1` |
| imports | MATCH (9/9 including `module_layout`) |
| exports | MATCH (none) |
| unresolved imports | 0 |
| CRC mismatches | 0 |
| protected-export failures | 0 |
| external signed-provider edges | 0 |
| stock re-sign envelope | 0 |

Every import resolves directly from vmlinux: `_printk`,
`alt_cb_patch_nops`, `kfree_skb_list_reason`, `module_layout`,
`param_array_ops`, `param_ops_charp`, `preempt_schedule_notrace`,
`register_qdisc`, and `unregister_qdisc`.

## Vendor-DLKM candidate

| Property | Result |
| --- | --- |
| output | `out/cellular-batch-01-candidate/vendor_dlkm.img` |
| SHA-256 | `640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958` |
| bytes | 143,986,688 |
| module inventory | 436, unchanged |
| intended changed modules | 1 (`rmnet_sch`) |
| unexpected changed modules | 0 |
| exact-stock cellular modules retained | 26 |
| ext4 `e2fsck -fn` | PASS |
| AVB footer/hashtree | PASS (`algorithm NONE`, stock descriptor format) |
| module image read-back | exact SHA-256 match |

The system-DLKM contract remains 46 entries with `wwan.ko` at entry 21,
zero missing entries, and zero stale built-in entries.

## Disposition

Static status: **READY FOR A FUTURE PHYSICAL BATCH-01 TEST**.

Physical cellular behavior is deliberately unclaimed. Before any later flash,
the vendor-DLKM must be backed up and the test must exercise registration,
RMNET IPv4/IPv6, routes, IP/DNS, data-call setup, airplane recovery,
Wi-Fi/cellular handoff, suspend/resume, reboot, and hotspot traffic where
equipment is available.

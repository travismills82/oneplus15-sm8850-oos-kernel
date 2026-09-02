# OOS 16.0.10.500 ACK 6.12.28 Android KABI repair audit

## Scope and qualified parent

This audit covers only the two Android/OxygenOS compatibility repairs applied
after the exact Linux 6.12.28 replay. It contains no Linux 6.12.29 or later
work and makes no change to a DLKM image, `vendor_boot`, VBMeta, userdata, or
metadata.

The physically qualified 6.12.27 lineage is anchored by:

| Field | Value |
|---|---|
| Qualified source commit | `20d91bf4ec43f6171bab445c4123350e64ab0883` |
| Static-validation commit | `28ac524da0e984b3ff371596237a7521ea61c641` |
| Qualification commit | `169fd4e9c3cbd6178bc40f4b6769ace1dff0bbe3` |
| Qualification tag | `oos16.0.10.500-ack-6.12.27-qualified` |
| Published 6.12.27 parent | `4a20f03c8b888995cb00c9d382374d262eb5d6f7` |
| Kernel release | `6.12.27-android16-5-o-g20d91bf4ec43-4k` |
| Tested `boot.img` SHA-256 | `8b5753c49a3899c0635558584ef6814e927662b459ecb4233761d532faad15b5` |
| Embedded `Image` SHA-256 | `ec61bb30e070b7eda06d4dc9607314f44f75b489eb970410e94546c2bfadb4b5` |

There is no surviving local or remote branch named for the qualified state;
the annotated qualification tag is the immutable branch-independent
qualification reference. Its report records two successful boots, existing
user0 `RUNNING_UNLOCKED`, WLAN at 6135 MHz with WPA3-SAE, LTE/RMNET,
Bluetooth/HID, NFC, camera, fingerprint, audio, USB, ZRAM, and five successful
deep-idle cycles.

The unmodified 6.12.28 experiment is preserved at `8227c049152e` on
`experiment/oos1610500-ack-6.12.28`. Compatibility work is isolated on
`experiment/oos1610500-ack-6.12.28-oos-compat`.

## Authoritative Android repair history

| Commit | Subject | KABI issue | Runtime intent | Action for this tree |
|---|---|---|---|---|
| `962d88304c3c` | `cpufreq: Fix setting policy limits when frequency tables are used` | Changes exported `cpufreq_table_index_unsorted()` from three to five arguments | Propagates a coherent explicit min/max snapshot through table selection | Retain its limit-aware logic behind a private helper while restoring the old export |
| `ad2b007ef43c` | `Revert "cpufreq: Fix setting policy limits when frequency tables are used"` | Android explicitly records the five-argument change as an ABI break | Defers the fix until it can be made ABI-safe | Used as authority for restoring the external prototype, but not copied blindly |
| `573b04722907` | `cpufreq: Avoid using inconsistent policy->min and policy->max` | Its follow-up requires the ABI-breaking argument propagation above | Uses `READ_ONCE`/`WRITE_ONCE` and ordered updates | Retained with an ABI-safe internal limit path |
| `a7e1300b95df` | `ANDROID: Revert "cpufreq: Avoid using inconsistent policy->min and policy->max"` | Explains that reverting only `962d88304c3c` can prevent raising max or lowering min | Android later reverts both pieces to avoid the partial-revert semantic bug | Used to require that this adaptation retain both pieces coherently |
| `65d3c570614b` | `xsk: Fix race condition in AF_XDP generic RX path` | Inserts `rx_lock` into `xsk_buff_pool`, shifting module-observable members | Serializes shared-UMEM generic receive at pool scope | Not retained because no low-risk ABI-safe storage/lifetime design exists in 6.12.28 |
| `834bfca1374a` | `Revert "xsk: Fix race condition in AF_XDP generic RX path"` | Android explicitly records the pool-layout change as an ABI break | Restores the prior per-socket lock and defers an ABI-safe race fix | Applied semantically and recorded as a separate downstream commit |

No unrelated later stable commit was imported.

## CPU-frequency compatibility

Commit `25801042c501` restores the exact external interface:

```c
int cpufreq_table_index_unsorted(struct cpufreq_policy *policy,
                                 unsigned int target_freq,
                                 unsigned int relation);
```

The exported wrapper reads the qualified `policy->min` and `policy->max`
members and calls the private, unexported five-argument
`cpufreq_table_index_unsorted_limits()` implementation. Internal core paths use
private explicit-limit helpers. Consequently:

- the external name, three-register calling convention, GPL export, return
  value, and source-facing helper contract match 6.12.27;
- sorted and unsorted tables retain explicit min/max selection internally;
- `CPUFREQ_RELATION_L`, `CPUFREQ_RELATION_H`, and `CPUFREQ_RELATION_C` retain
  the 6.12.28 selection logic;
- `cpufreq_driver_resolve_freq()` retains coherent `READ_ONCE` snapshots and
  max-below-min handling;
- `cpufreq_set_policy()` retains ordered `WRITE_ONCE` updates, so maximum and
  minimum limits can move in either direction without the partial-revert
  clamping bug described by `a7e1300b95df`.

Arm64 disassembly proves the external wrapper consumes only `x0`, `w1`, and
`w2`; it moves relation into `w4`, loads min/max from policy offsets `0x34` and
`0x38`, and calls the private limit-aware helper. No CRC or vermagic value was
patched.

| Build | Prototype | CRC | Export |
|---|---|---|---|
| Qualified 6.12.27 | three arguments | `0x4ecab20e` | `EXPORT_SYMBOL_GPL` |
| Plain 6.12.28 | five arguments | `0xf924eade` | `EXPORT_SYMBOL_GPL` |
| Compatible 6.12.28 | three arguments | `0x4ecab20e` | `EXPORT_SYMBOL_GPL` |

No module in the retained stock 1,020-module universe imports this symbol, but
the enforced common ABI target verifies the GKI contract independently.

## AF_XDP compatibility

The qualified `xsk_buff_pool` has no Android KABI reserve at the insertion
point. Reusing a different existing lock does not cover all shared-UMEM pool
users, and a side-car object would introduce unproven allocation, lifetime,
lookup, and teardown behavior. The safe generation-5 choice is therefore the
official Android strategy in `834bfca1374a`: restore the per-`xdp_sock` lock
and defer the shared-UMEM race fix until an ABI-safe design exists.

This preserves the qualified ABI but does not preserve the broader
shared-UMEM serialization added by `65d3c570614b`. The deferral is explicit;
it is not hidden with a type-string rule.

| Member | Qualified 6.12.27 | Plain 6.12.28 | Compatible 6.12.28 | Result |
|---|---:|---:|---:|---|
| `rx_lock` | not in pool | 80 | not in pool | qualified layout restored |
| `free_list` | 80 | 88 | 80 | match |
| `xskb_list` | 96 | 104 | 96 | match |
| `heads_cnt` | 112 | 120 | 112 | match |
| `queue_id` | 116 | 124 | 116 | match |
| structure size | 256 | 256 | 256 | match |
| structure alignment | 8 | 8 | 8 | match |
| member count | 33 | 34 | 33 | match |

`pahole` output for the full qualified and compatible structures is
byte-for-byte identical. The ordinary 6.12.28 size happens to remain 256 due
to tail padding; the middle-member offsets are nevertheless incompatible.

The four transitive export CRCs contaminated by the changed pool type also
return naturally to their qualified values:

| Symbol | Qualified / compatible CRC | Plain 6.12.28 CRC |
|---|---|---|
| `__ethtool_get_link_ksettings` | `0xc71fd974` | `0xdeb9e0c7` |
| `ethtool_op_get_link` | `0x9229d1db` | `0x8bbeb86b` |
| `ethtool_op_get_ts_info` | `0x277e0608` | `0x16dcd515` |
| `sk_filter_trim_cap` | `0xa8366e5b` | `0xb9e79bbe` |

## Stock module blocker progression

The cpufreq repair restores the common ABI but does not change the retained
stock-module count because none of those binaries imports the cpufreq symbol.
The XSK repair removes all 16 mismatch edges and all 12 active blockers.

| Stage | Active compatible | Dormant | Active blockers | CRC mismatch edges |
|---|---:|---:|---:|---:|
| Plain 6.12.28 | 972 | 36 | 12 | 16 |
| After cpufreq repair | 972 | 36 | 12 | 16 |
| After XSK repair | 984 | 36 | 0 | 0 |
| Final committed source | 984 | 36 | 0 | 0 |

The 12 active blockers were:

| Module | Partition | Mismatching symbol family | Cause | Final result |
|---|---|---|---|---|
| `8021q.ko` | system_dlkm | link / link-ksettings | XSK type propagation | compatible |
| `aqc111.ko` | system_dlkm | link | XSK type propagation | compatible |
| `ax88179_178a.ko` | system_dlkm | link / timestamp-info | XSK type propagation | compatible |
| `bluetooth.ko` | system_dlkm | socket filter | XSK type propagation | compatible |
| `cdc_ether.ko` | system_dlkm | timestamp-info | XSK type propagation | compatible |
| `cdc_ncm.ko` | system_dlkm | timestamp-info | XSK type propagation | compatible |
| `r8152.ko` | system_dlkm | link | XSK type propagation | compatible |
| `rtl8150.ko` | system_dlkm | link | XSK type propagation | compatible |
| `slcan.ko` | system_dlkm | timestamp-info | XSK type propagation | compatible |
| `usbnet.ko` | system_dlkm | link / timestamp-info | XSK type propagation | compatible |
| `vcan.ko` | system_dlkm | timestamp-info | XSK type propagation | compatible |
| `mac80211.ko` | vendor_dlkm | link | XSK type propagation | compatible |

The stock vendor-boot `mac80211.ko` copy had the same link CRC mismatch but is
dormant under the supported load policy; it is also compatible after the
repair. No third ABI blocker remains.

## Security and scope conclusion

The compatibility delta from the preserved static-bad 6.12.28 commit consists
of exactly eight common-kernel files: four cpufreq files and four XSK files.
It does not change configuration, module lists, certificates, signing,
MODVERSIONS, GENDWARFKSYMS, protected exports, ABI/KMI enforcement, FBE,
DLKM packaging, or firmware payloads.

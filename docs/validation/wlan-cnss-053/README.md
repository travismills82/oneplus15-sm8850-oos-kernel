# Canoe WLAN/CNSS .053 experiment

## Frozen input contract

This experiment begins at `feature/controlled-v1-test3` commit
`dd4c70215e2dd3e7dfccd5aac55cb91564df77b8`. The physical TEST3 payloads
remain the rollback contract:

| Payload | SHA-256 |
| --- | --- |
| `boot.img` | `25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313` |
| `system_dlkm.img` | `edebc94818e6fa4e214d58fd82fe46f6c513fc9850b3e7b77caf076a12270f05` |
| `vendor_dlkm.img` | `24e66015a3e4ea3583f895d529008d8c7c3706d7bc506ef550df936935127b80` |

`vendor_boot` and VBMeta remain exact stock. Slot A is unbootable and is not
an A/B fallback.

## Source provenance

The authoritative OnePlus OSS snapshots are:

| Generation | Product snapshot | Commit |
| --- | --- | --- |
| `.037` | `oneplus/sm8850_b_16.0.0_oneplus_15` | `5ab2a689ff87d7d28c511f1762cf41c1b90d965a` |
| `.053` | `oneplus/sm8850_b_16.0_oneplus_15t` | `d447f713d6403f707a2910383495f4ada98cfa4d` |

Before import, all five current WLAN component trees exactly match their
official `.037` tree objects:

| Tree | `.037` object | `.053` object |
| --- | --- | --- |
| `fw-api` | `1c39be3908ad43a5db5b35149177e425b2deca9e` | `2a35c44f8badeb0df0c16cc9bef43f3d8f26ac24` |
| `platform` | `2484ace413ae78e55cc87e02b71c558cb36fde46` | `88b2e71dc1409062504072f8f7c046072e5508df` |
| `qca-wifi-host-cmn` | `13796b9b38426319e3e4b3138dedbfb3804d792e` | `e61664cceee4307c0383a3cb82f1b3386af4cc0a` |
| `qcacld-3.0` | `1c0818627414c3d9fda3395fb1daf6b7168cae5e` | `d00300cc4e7dbc213e1d4f81a51b62deec93299d` |
| `wlan-devicetree` | `d0391ac9599cecb6feef08e5037c69a95f552be3` | `2a0863ba7091bf96822360e4ea6da35e6ae0df4b` |

The direct `.037` to `.053` WLAN snapshot delta contains 844 paths: 306
added, 536 modified, and two renames. `utils/sigma-dut` is userspace test
source and is not a kernel-module input. Product WLAN device-tree changes are
not imported because this experiment retains the shipping Canoe DTBO and the
Canoe module build does not consume those product DTS paths.

The tracked `qcacld-3.0/api` and `qcacld-3.0/cmn` symlinks bind the Peach
build to `fw-api` and `qca-wifi-host-cmn`, respectively. Those trees therefore
must move with qcacld; mixing their generations is forbidden.

## Running .037 module contract

`tools/audit-wlan-cnss-generation.py` inspected the actual TEST3
vendor-DLKM stage and a live slot-B `/proc/modules` snapshot. The detailed
reports under `wlan037/` contain:

- 12 scoped modules;
- 2,001 versioned imports;
- 610 exported symbols and CRCs;
- 10 module parameters;
- firmware declarations and aliases; and
- source path, build target, module hash, signer, vermagic, srcversion, and
  observed runtime state for every module.

The active `.037` delivery is deliberately hybrid. `cfg80211`, `mac80211`,
and Peach-v2 are controlled source builds. The active CNSS platform modules
are exact OxygenOS stock binaries. `wonder` is the exact stock payload with a
controlled-v1 signature. `icnss2` is packaged but was not loaded on the
observed Peach-v2 runtime. Shared `smem_mailbox` is exact stock.

This distinction is important: the `.053` experiment may replace a CNSS
module only when the new consumer/provider contract requires it or when it is
part of the reviewed active `.053` platform closure. Exact-stock IPA/GSI/RMNET
providers remain preferred whenever their symbol and CRC contracts satisfy
the new WLAN consumer.

## Mandatory gates

The experiment must preserve:

- `CONFIG_CFG80211=m` and `CONFIG_MAC80211=m`;
- the 46-entry system-DLKM load policy with `wwan.ko` at entry 21;
- all 27 exact-stock vendor cellular modules;
- signed-regdb, SAR, DFS, thermal, country-code, and 6 GHz policy;
- external Peach firmware and board/config data; and
- normal signature, MODVERSIONS, and protected-export enforcement.

No device write is authorized by this audit. A future candidate may proceed
only after static closure and firmware checks pass and the hardened TWRP
helper passes a dry run with a verified TEST3 restore image.

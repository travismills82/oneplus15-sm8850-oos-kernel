# Matched WLAN vendor-DLKM closure

## Scope and status

This documents the source-controlled WLAN vendor-DLKM closure for Canoe
(OnePlus 15 / CPH2747) rebuilt from the exact payload lineage that is running
on the device:

```text
bd70777d3d2cbb44e758d8ad36c264b18b7b69ab
security: backport NFC LLCP connection-confirm UAF race fix
```

It is a compatibility baseline, not a WLAN feature upgrade. In particular, it
does not change CFG80211, MAC80211, the Peach-v2 source revision, or any
Oplus/Qualcomm WLAN feature selection. The Canoe DDK configuration keeps
`CONFIG_CFG80211=m` and `CONFIG_MAC80211=m`; `CONFIG_RFKILL=y` remains enabled.

Status: **REBUILD REQUIRED**. The earlier candidate was built from the r7
release commit while the phone was running the later `gbd70777d3d2c` payload.
It booted Android and then shut down, so it was restored from verified backups
and is rejected. This document deliberately records no validation result until
the complete closure has been rebuilt from the exact payload/config lineage and
tested with persistent host-side failure logging.

## Source-built provider set

The following targets must be built directly from the exact payload source tree.
The Peach-v2 target is the Canoe-specific DDK target and retains its existing
Oplus feature defines and generated configuration.

Before staging, the packager reads the matching build's generated
`include/config/kernel.release` and rejects every source replacement whose
`vermagic` does not begin with that exact release. This blocks stale DDK output
from entering the candidate even when its exported CRCs happen to match.

| Module | Source target |
| --- | --- |
| `cfg80211` | `//soc-repo:canoe_perf/net/wireless/cfg80211` |
| `mac80211` | `//soc-repo:canoe_perf/net/mac80211/mac80211` |
| `qca_cld3_peach_v2` | `//vendor/qcom/opensource/wlan/qcacld-3.0:canoe_perf_qca_cld_peach-v2` |
| `cnss2` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss2` |
| `cnss_nl` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_nl` |
| `cnss_plat_ipc_qmi_svc` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_plat_ipc_qmi_svc` |
| `cnss_prealloc` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc` |
| `cnss_utils` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_utils` |
| `wlan_firmware_service` | `//vendor/qcom/opensource/wlan/platform:canoe_perf_wlan_firmware_service` |
| `gsim` | `//vendor/qcom/opensource/dataipa:canoe_perf_gsim` |
| `ipam` | `//vendor/qcom/opensource/dataipa:canoe_perf_ipam` |
| `rmnet_mem` | `//vendor/qcom/opensource/datarmnet-ext/mem:canoe_perf_rmnet_mem` |
| `smem_mailbox` | `//vendor/qcom/opensource/data-kernel/drivers/smem-mailbox:canoe_perf_smem_mailbox` |
| `wonder` | `//vendor/oplus/kernel/wifi:wonder` |

The normal build outputs contain DWARF. The staging process uses the standard
module-install equivalent, `llvm-objcopy --strip-debug`, before signing a
*copy* of each output. It does not alter the source output or the stock image.

## ELF and MODVERSIONS contract

Each stripped source-built module is compared to its exact stock `vendor_dlkm`
peer. The comparison includes every `__versions` import and every exported
symbol CRC. The rebuild is accepted only when it reports zero replacement
contract failures, import CRC mismatches, and unresolved imports.

When that gate passes, the source providers preserve the ABI expected by the
retained stock modules. Their module names and dependency interfaces remain
unchanged, so the stock Android flat-layout `modules.load`, `modules.dep`,
`modules.alias`, and `modules.softdep` metadata are retained rather than
regenerated from an incompatible `/lib/modules/<release>` layout.

## Protected-export signing closure

`CONFIG_MODULE_SIG=y`, `CONFIG_MODULE_SIG_PROTECT=y`, `CONFIG_MODVERSIONS=y`,
and `CONFIG_GENDWARFKSYMS=y` remain enabled. `CONFIG_MODULE_SIG_FORCE` is not
changed. Moving a provider to a project-signed copy requires every stock
consumer of its protected export to be signed as well; otherwise protected
export enforcement correctly rejects the mix.

The matching Image build supplies its build certificate and keeps the configured
OOS stock GKI certificate as a trusted key. The exact build-certificate
fingerprint is recorded in the generated staging manifest; no private key is
checked into this repository.

The expected vendor-DLKM closure contains 29 project-signed modules:

- The 14 source-built providers above.
- Exact stock binaries, re-signed only because they consume a newly signed
  protected provider: `bt_fm_swr`, `btfm_slim_codec`, `btpower`, `icnss2`,
  `ipanetm`, `qca_cld3_kiwi_v2`, `qca_cld3_wcn7750`, `rmnet_aps`,
  `rmnet_core`, `rmnet_ctl`, `rmnet_offload`, `rmnet_perf`,
  `rmnet_perf_tether`, `rmnet_shs`, and `rmnet_wlan`.

No protected-export exception, weak symbol, fake provider, or signature bypass
is used.

## Image staging

`tools/validate-matched-wlan-vendor-dlkm.py` overlays the 14 replacements on
the verified stock vendor-DLKM inventory, checks import resolution against the
final closure and retained external module areas, and computes the minimal
signing closure. It emits TSV reports for replacement contracts, import
resolution, signing closure, and external-boundary edges.

`tools/stage-matched-wlan-vendor-dlkm.sh` then:

1. copies the raw, verified stock `vendor_dlkm` image;
2. replaces exactly the 29 closure modules with stripped and signed staged
   copies;
3. preserves the image format, filesystem layout, and partition size;
4. runs `e2fsck -fn`; and
5. reads every replacement back through `debugfs` and compares SHA-256 hashes.

The candidate must retain the stock 143,986,688-byte partition size. Filesystem
validation must complete cleanly and every changed module must have a matching
read-back hash before device staging is considered.

## Retained vendor-boot boundary

The stock vendor boot ramdisk includes dormant `mac80211` and `wonder` module
copies. They would consume 129 protected-provider imports from the new closure:

| Retained vendor-boot consumer | Signed provider | Imports |
| --- | --- | ---: |
| `mac80211` | `cfg80211` | 110 |
| `wonder` | `mac80211` | 13 |
| `wonder` | `cfg80211` | 6 |

The inspected vendor-boot `modules.load*`, init files, scripts, and
configuration files do not request these copies in the normal OOS boot path;
the normal vendor-DLKM `modules.load` requests the vendor-DLKM counterparts.
Nevertheless, this is a real external ABI/signing boundary. The validator's
`--fail-external-signed-provider-edges` mode rejects the candidate while these
129 edges exist. A future controlled-vendor-boot phase must rebuild or sign
those exact vendor-boot consumers before any path that loads them is supported.

## Acceptance boundary

This closure is suitable only as the next static artifact for a controlled
vendor-DLKM experiment. AVB metadata and vendor boot remain unchanged. Any
repeat device staging must start a host-side persistent dmesg/logcat/periodic
state capture before the reboot, preserve verified known-good backups, and
record the post-failure recovery evidence before another candidate is tried.

Run `tools/capture-matched-wlan-boot.sh` before any device staging. It writes
only host-side evidence and deliberately performs no ADB write, property change,
reboot, flash, or recovery action.

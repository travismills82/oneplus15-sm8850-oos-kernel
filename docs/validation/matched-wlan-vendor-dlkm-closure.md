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

Status: **DEPENDENCY-RECONCILIATION ARTIFACT READY FOR RETEST**. The earlier candidate
was built from the r7 release commit while the phone was running the later
`gbd70777d3d2c` payload. It booted Android and then shut down, so it was
restored from verified backups and is rejected.

The first exact-lineage staged test booted Android, mounted the custom EROFS
`system_dlkm`, and mounted the custom `vendor_dlkm`, but Wi-Fi could not load.
The retained stock image was restored byte-for-byte afterwards. Static
inspection found that the stager's `debugfs write` operation had replaced each
module inode with mode `0664` and no `security.selinux` attribute instead of
the stock mode `0644` and
`u:object_r:vendor_file:s0\000`. That makes the replacements unavailable to
the confined `vendor_modprobe` service before any module CRC or Peach-v2
compatibility result can be inferred. The candidate is therefore rejected as a
packaging failure, not as an ABI failure.

The stager now preserves and verifies the original mode, uid, gid, and exact
SELinux xattr for every replaced module. The rebuilt artifact passes the full
post-sign closure audit (zero unresolved imports, zero CRC mismatches, and zero
replacement-contract failures), `e2fsck`, and local AVB hashtree verification.
It still requires a new physical test.

### Metadata-preserving retest result

The metadata-preserving candidate was physically staged to slot `_b` on
2026-08-16 with a persistent host-side dmesg/logcat/snapshot capture. It
booted Android and the project-signed CNSS/IPA prerequisites loaded, proving
that the restored `vendor_file` label and module signatures were accepted.
However, `cfg80211`, `mac80211`, and `qca_cld3_peach_v2` were absent from
`/proc/modules` when Wi-Fi was enabled. The stock Peach retry then reported
the expected missing cfg80211 exports, including `cfg80211_scan_done`,
`cfg80211_connect_done`, and `wiphy_register`.

This was a second packaging failure, not a CRC or protected-export failure.
The candidate retained the stock flattened `modules.dep` row:

```text
/vendor/lib/modules/cfg80211.ko: /system/lib/modules/rfkill.ko
```

but the exact matching Image has `CONFIG_RFKILL=y`, and the matching custom
system-DLKM does not contain `rfkill.ko`. Android's vendor modprobe therefore
could not satisfy cfg80211's stale file dependency before Peach was inserted.
The test slot was restored in recovery in dependency-first order, with the
following full-partition SHA-256 read-back proofs before boot was restored:

```text
vendor_dlkm_b  40d4bd03e9d315aac562234019f6db192617bb1ab65532157b81022ebc7330e6
system_dlkm_b  bcd4b14f940574bd2f55c967dfb4cec1e17a008197bb769dc05fd5e13c8670ce
boot_b         86eba62f4f93f02aaacda89ef903c91a3e531575aba1cf253ce70e0887b38d1f
```

The restored kernel immediately loaded the stock modular WLAN chain and
reconnected to the saved 6 GHz WPA3 network. Persistent evidence, including
the failed candidate's targeted metadata/dmesg capture and the final restored
snapshot, is stored under:

```text
/home/travis/Android/oneplus15-matched-wlan-gbd70777-live-captures/
20260816T174537Z-metadata-fixed/
```

The next artifact reconciles vendor `modules.dep` against the exact
`system_dlkm_staging_archive.tar.gz` and the matching `modules.builtin` file.
It prunes only providers compiled into vmlinux (53 edges, including the
critical cfg80211-to-rfkill edge), retains 37 real system-DLKM dependencies,
and rejects any missing provider. The generated `modules.dep` is itself
replaced with its original mode, uid, gid, and `security.selinux` label and is
read back from the final image for hash and metadata verification.

The resulting dependency-reconciled artifact is:

```text
vendor_dlkm.img
SHA-256: bdfe5ace6796cc2341720d58bdbfde0fb7099201aa469cc6fe016c4519cf04ea
size:    143,986,688 bytes
```

Its complete post-sign validator result is PASS with zero unresolved imports,
zero CRC mismatches, and zero replacement-contract failures. The exact
`gbd70777d3d2c` build was then rerun through `canoe_perf_dist`,
`kernel_aarch64_abi`, `kernel_aarch64_abi_kmi_symbol_checks`, and
`kernel_aarch64_abi_diff`; all passed with the existing empty ABI report.

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
unchanged, so the stock Android flat-layout `modules.load`, `modules.alias`,
and `modules.softdep` metadata remain intact. `modules.dep` is not blindly
copied: it is reconciled against the exact matching system-DLKM module archive
and vmlinux built-in list. This is a narrow path correction, not a conversion
to an incompatible `/lib/modules/<release>` layout.

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
   copies while preserving and read-back-verifying the stock module mode, uid,
   gid, and `security.selinux` xattr;
3. compares every `/system/lib/modules/*.ko` dependency in the stock flat
   `modules.dep` against the exact matching system-DLKM staging archive and
   `modules.builtin`, removing only dependencies supplied by vmlinux,
   retaining real system-DLKM module paths, and rejecting all unknown paths;
4. replaces the reconciled `modules.dep` while preserving and read-back-
   verifying its stock mode, uid, gid, and `security.selinux` xattr;
5. regenerates the partition-local unsigned AVB hashtree, FEC, and footer
   from the modified ext4 payload while preserving stock geometry, salt,
   properties, rollback fields, and partition size;
6. verifies the regenerated footer and hashtree with `avbtool`;
7. runs `e2fsck -fn`; and
8. reads every replacement and the reconciled metadata back through `debugfs`
   for SHA-256 and inode/xattr comparison.

The candidate must retain the stock 143,986,688-byte partition size. Filesystem
validation must complete cleanly and every changed module must have a matching
read-back hash before device staging is considered. Retaining the stock AVB
hashtree after changing modules is invalid and is a hard staging failure.

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
vendor-DLKM experiment. Its partition-local footer is internally valid, but
parent/top-level AVB metadata and vendor boot remain unchanged; that is valid
only under the separately verified unlocked-device development AVB policy.
Any repeat device staging must start a host-side persistent dmesg/logcat/
periodic state capture before the reboot, preserve verified known-good backups,
and record the post-failure recovery evidence before another candidate is tried.

Run `tools/capture-matched-wlan-boot.sh` before any device staging. It writes
only host-side evidence and deliberately performs no ADB write, property change,
reboot, flash, or recovery action. Its parent supervisor deliberately survives
ADB stream disconnects and flushes each completed snapshot; run it from a
persistent host shell (for example `setsid`) through the recovery/boot cycle.

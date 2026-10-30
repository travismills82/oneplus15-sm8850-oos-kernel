# ColorOS .102 built-in versus stock-module ownership

This complements the complete1070-file [stock module inventory](coloros17-module-inventory.md).
No driver was converted to built-in, no configuration was expanded and no
module was moved between partitions.

## Compiled kernel evidence

The qualified common build's `modules.builtin` lists643 built-in Kbuild units,
SHA256 `ab0c905229a6507d032a22ca665792f772961e824353aefc642865ff9b01acb8`.
The adjacent generated config and Module.symvers match the frozen production
hashes in the [baseline report](coloros17-qualified-baseline.md). The complete
list is preserved as [643 built-in units](coloros17-built-in-units.tsv).

Names ending `.ko` in that particular build metadata identify units linked
into vmlinux, not emitted loadable modules. The list is not every core C
translation unit and is not a claim that every compiled platform driver
binds hardware on this phone. Generic GKI includes unrelated-platform drivers;
keep compiled presence separate from actual DT/probe ownership.

| Layer | Qualified ownership / evidence |
|---|---|
| F2FS, fscrypt, block inline crypto, device mapper | Built into the boot kernel; enabled config plus prior F2FS/inlinecrypt mount observations |
| Netdevice/TCP/netfilter/conntrack core | Built into the boot kernel; vendor networking providers remain modules |
| USB DWC3 generic core | Built into vmlinux; exact private layout1488 bytes/xhci384 retained |
| Qualcomm/OPlus DWC3 MSM glue | Stock vendor_boot/vendor_dlkm module copies; do not confuse generic built-in dwc3-qcom with the actual downstream glue |
| UFS generic core | Built into vmlinux; Qualcomm UFS/ICE/SCM/PHY providers remain stock modules |
| Bluetooth/HIDP, PPP, RFKILL, SLHC | Stock system_dlkm modules; not built in and not shadowed by a custom loader |
| cfg80211/mac80211, WLAN/CNSS hardware | Stock modules; the installed chipset and normal/recovery load order decide provider selection |
| DRM/sound/thermal/CPUFreq frameworks | Core framework configuration built in; Qualcomm/OPlus hardware providers are separately inventoried modules |
| KGSL, display, camera, audio/DSP, IPA/RMNET, SecureMSM, touch and OPlus hardware | Primarily stock vendor_dlkm/vendor_boot modules; exact partition-qualified entries and duplicates are in the inventory |

Common and Canoe module configs are distinct. For example the common GKI config
contains built-in generic DWC3 helpers, while downstream driver/module flags
come from the existing Canoe/OPlus fragments. Never infer an actual hardware
provider by grepping only one config or by a source filename.

## First network targets' actual stock placement

`cnss2.ko`, `cnss_prealloc.ko` and `rmnet_core.ko` each occur only in stock
vendor_dlkm, with normal load classification. The preserved vendor modules.load
contains cnss2/prealloc requests more than once; preserve that exact metadata
rather than deduplicating it as cleanup. RMNET core is listed at line586.

If a bounded update to one of these providers passes all gates, its minimal
partition requirement is vendor_dlkm. That is not yet permission or proof to
build/flash an image: the signed top-level AVB descriptor gate remains.
No change to system_dlkm or vendor_boot follows merely from building these
network modules. Dependency modules built only to obtain legitimate compiler
metadata are not automatically package replacements.

No phone is currently visible. Previous runtime evidence identifies691 loaded
module names; full live DT binding and current provider paths were not newly
probed. No actual runtime test of new modules is claimed.

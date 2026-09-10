# SM8850 Canoe inline-encryption path

## Runtime path

```
F2FS encrypted inode and fscrypt v2 policy
  -> built-in fscrypt inline_crypt.c
  -> built-in blk-crypto profile and bio crypt context
  -> built-in dm-default-key / device-mapper crypto-profile forwarding
  -> built-in UFS core crypto request preparation
  -> stock vendor_boot ufshcd-crypto-qti.ko
  -> stock vendor_boot qcom_ice.ko
  -> stock vendor_boot qcom-scm.ko
  -> Qualcomm secure-world ICE/HWKM service
```

## Component classification

| Component | Classification | Source or binary identity |
|---|---|---|
| F2FS | `BUILT_IN_COMMON` | `kernel_platform/common/fs/f2fs/` |
| fscrypt | `BUILT_IN_COMMON` | `kernel_platform/common/fs/crypto/` |
| blk-crypto | `BUILT_IN_COMMON` | `kernel_platform/common/block/blk-crypto*.c` |
| dm-default-key | `BUILT_IN_COMMON` | `kernel_platform/common/drivers/md/dm-default-key.c` |
| device-mapper crypto-profile forwarding | `BUILT_IN_COMMON` | `kernel_platform/common/drivers/md/dm-table.c` |
| UFS core crypto | `BUILT_IN_COMMON` | `kernel_platform/common/drivers/ufs/core/ufshcd-crypto.c` |
| QTI UFS crypto | `VENDOR_BOOT_MODULE` | `ufshcd-crypto-qti.ko`; source at `kernel_platform/soc-repo/drivers/ufs/host/ufshcd-crypto-qti.c` |
| Qualcomm UFS host | `VENDOR_BOOT_MODULE` | `ufs-qcom.ko`; source at `kernel_platform/soc-repo/drivers/ufs/host/ufs-qcom.c` |
| Qualcomm ICE | `VENDOR_BOOT_MODULE` | `qcom_ice.ko`; source at `kernel_platform/soc-repo/drivers/soc/qcom/ice.c` |
| Qualcomm SCM | `VENDOR_BOOT_MODULE` | `qcom-scm.ko`; source at `kernel_platform/soc-repo/drivers/firmware/qcom/` |
| HWKM/ICE key service | `SECURE_WORLD_FIRMWARE` | immutable OxygenOS firmware |

The `soc-repo` config sets the four device-specific components above to `m`,
and `modules.list.msm.canoe` contains `qcom-scm.ko`,
`ufshcd-crypto-qti.ko`, `qcom_ice.ko`, and `ufs-qcom.ko`.  The qualified
firmware inventory classifies all four stock binaries as normal and recovery
load-path modules.

## Hardware declaration and key semantics

`kernel_platform/qcom/opensource/devicetree/qcom/canoe.dtsi` declares the UFS
controller at `ufshc@1d84000`, embeds the ICE register range, and sets
`qcom,ice-use-hwkm`.  The QTI crypto-profile implementation advertises only
AES-256-XTS and hardware-wrapped keys, retains a maximum eight-byte DUN, and
derives the fscrypt software secret through the ICE/SCM chain.

The generic common ICE driver is not the active Canoe provider.  Post-baseline
generic ICE fixes concerning the dedicated ICE platform-device probe race and
explicit `iface` clock votes cannot be claimed fixed by changing only the
common boot image.  Porting those changes to the stock QTI module path would
require an exact-source vendor module update and a different payload/AVB
project; they are therefore recorded as `DEFERRED_DEPENDENCY` here.

## Boot-only compatibility boundary

This phase may modify only built-in common-kernel behavior while retaining the
stock module-visible contract.  It must not change the four device-specific
module binaries, their load order, their exported/imported ABI, device tree,
secure-world firmware, or any DLKM/vendor_boot container.  Every selected
common-kernel change must pass the complete 1,020-module oracle and a physical
TWRP plus Android existing-user0 test.

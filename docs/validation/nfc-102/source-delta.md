# NXP NFC vendor controller .089 to .102 source delta

## Scope and provenance

- Qualified base: `feature/controlled-v1-wlan053` at
  `e7444f14cbbd587e528fab386ad886ee474138c6`.
- Current generation:
  `AU_LINUX_ANDROID_LA.VENDOR.16.2.0.R1.11.00.00.1277.089`.
- Target generation:
  `AU_LINUX_ANDROID_LA.VENDOR.16.2.0.R1.11.00.00.1277.102`.
- Official newer same-SM8850 source identity:
  `d447f713d6403f707a2910383495f4ada98cfa4d`.
- Imported runtime commit: `a6089885e401fc045bb842727f9861be05f1fab6`.

No 15T or Pad branch was merged. The active driver runtime source was compared
against the official `.102` tree; only the one changed active source file was
imported. Its `.089` SHA-256 is
`6803f1c8981c8ba97f573c1eb58bb1b893357d0709bbeb7d14f835a5345e6daa` and
its imported `.102` SHA-256 is
`07f02e268368f71bc3a8fd05894cbf8b71c4bfd280b9aad848418445a9cd469f`.

## Active controller proof

The frozen WLAN053 captures prove this normal path:

```text
OxygenOS NXP NFC HAL (Product: SN220)
        |
        v
    /dev/nq-nci
        |
        v
nxp_nci / nxp-nci.ko
        |
        v
qcom,sn-nci 21-0028
        |
        v
CPH2747 SN220 over QUPv3 SE21 I2C
```

`/proc/modules` contains `nxp_nci`; boot dmesg records
`nfc_i2c_dev_init`, `qcom,sn-nci 21-0028`, parsed IRQ/VEN/CLKREQ GPIOs,
and a successful probe. OxygenOS reports `ro.strongbox.model=SN220`, and the
NXP HAL records the SN220 firmware path `/data/vendor/nfc/sn220u.bin`.

The simultaneously loaded ST, ST54SE, and THN31 modules have no evidence of
binding the `qcom,sn-nci` node. They remain byte-identical and are not part of
the replacement set.

## Complete active runtime delta

| Path | Change | Classification | Canoe decision |
| --- | --- | --- | --- |
| `vendor/nxp/opensource/driver/nfc/i2c_drv.c` | Treat every nonzero `wait_event_interruptible()` return as interruption and remove unreachable/incorrect post-wait branches | RACE/LIFETIME; ERROR HANDLING; NCI TRANSPORT | IMPORT; active SN220 I2C read path |

The wait API returns zero when the condition becomes true and a negative
error when interrupted. The `.089` code checked `ret < 0` and then contained
branches for `ret == 0 && irq_enabled` and `ret > 0`; `.102` uses the API's
actual return contract directly. This is a correctness/lifetime hardening
change. It is not labelled a security fix because the source delta alone does
not demonstrate an exploitable memory-safety condition.

## Excluded newer-drop changes

The broader official drop also changes build selector/target wiring and adds
newer/foreign device-tree target coverage. Those files are not consumed by
the active CPH2747 runtime fix and were deliberately excluded:

| Change family | Classification | Decision |
| --- | --- | --- |
| Additional platform/target selectors | BUILD ONLY | DROP; retain the proven Canoe `.089` Kleaf target definition |
| Parrot and other new board targets | FOREIGN DEVICE | DROP |
| New board IDs/MSM IDs in NFC DT targets | DEVICE TREE / FOREIGN DEVICE | DROP |
| Foreign GPIO, IRQ, regulator, pinctrl, clock, or controller nodes | FOREIGN DEVICE | DROP |

There are no `.102` changes to the active UAPI header, ioctl handlers, device
name, module aliases, exported symbols, module parameters, or firmware
declarations in this experiment.

## Device-tree contract

The CPH2747 tree remains authoritative. The existing node already supplies:

- `compatible = "qcom,sn-nci"`;
- I2C bus 21, address `0x28`;
- IRQ GPIO 75, VEN GPIO 34, CLKREQ GPIO 35, VBAT GPIO 174;
- secure-zone enablement;
- `nfc_bob1_cell` NVMEM data; and
- the existing active/suspend pinctrl states.

The imported runtime file introduces no required property or compatible
string. No DT source or DTBO changes are part of the candidate.

## Kernel and module contract

Release-contract architecture 2 treats this as an external DDK-only source
identity. A clean fail-closed build reproduced the qualified kernel contract:

- kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`;
- `.config` differences: 0;
- `Module.symvers` differences: 0;
- existing exported CRC differences: 0;
- Image functional differences: 0;
- ABI report: empty; and
- KMI check: pass.

The `.102` module preserves all 81 import CRCs and has no exports. It does not
import Linux NFC/NCI-core exports directly; the stock HAL uses the NXP
character-device transport while `CONFIG_NFC=y` remains unchanged. Every
import resolves against the qualified Image, retained vendor-DLKM, or retained
vendor-boot providers.

## Userspace and firmware contract

The only code change is inside the I2C wait path. `/dev/nq-nci`, the
`qcom,sn-nci` alias, ioctl UAPI, NCI framing, and SN220 firmware path remain
unchanged. No firmware is embedded or declared by the module. Therefore the
stock OxygenOS 16.0.9.400 NFC HAL and firmware delivery contract are unchanged
statically; tag, Wallet, and secure-element behavior still require physical
qualification.

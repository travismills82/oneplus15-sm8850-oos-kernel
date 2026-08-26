# Candidate B05: USB gadget UDC lifetime hardening static validation

## Decision

**READY — CANDIDATE B05 PHYSICAL TEST**

Candidate B05 cumulatively contains physically qualified B01 Binder lifetime,
B02 BPF per-CPU bounds, B03 eventpoll lifetime, and B04 AF_PACKET fanout
lifetime hardening plus one USB gadget UDC lifetime fix. It does not advance
ACK, change configuration, modify a module provider, adapt a DLKM, convert a
module to a built-in, alter device tree, or require new firmware or userspace.
The B05 payload has not been flashed.

## Source identity and reachability

| Field | Value |
|---|---|
| Qualified B04 parent | `e5896aba2186c2f47cfc5d45d9d1f26cbff943eb` |
| B04 tested runtime source | `2f2631b951ced2ef05a4a9643610954b26736bcd` |
| B04 tested boot | `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3` |
| B05 branch | `experiment/oos1610500-custom-r53-b05` |
| B05 runtime source | `515d73a3d5f436bb3b67d36ef1be44fafd22e0ae` |
| Kernel release | `6.12.23-android16-5-o-g515d73a3d5f4-4k` |
| Intended runtime path | `kernel_platform/common/drivers/usb/gadget/udc/core.c` |
| Unexpected runtime groups | 0 |

The qualified B04 freeze is an ancestor of B05. The relevant Canoe build has
`CONFIG_USB_GADGET=y`, `CONFIG_USB_CONFIGFS=y`, and
`CONFIG_USB_CONFIGFS_F_FS=y`. Preserved B04 physical logs show Android's
configfs gadget and ADB paths bind a UDC, making this a confirmed active path
rather than a merely compiled driver.

## Value and provenance

B05 carries controlled commit
`e8092bd45984629dc89981a4443bfcb22060a933`, the exact Android Common Kernel
commit `7d66c85503045527d51998c0f5a249c3fd209670`, derived from upstream
`67e511d2989eb1c8c588b599ce2fcc6bb8e6f7ea`.

The firmware-native r53 implementation has the decoupled gadget/UDC lifetime
but lacks the reference pin. B05 keeps a UDC device reference until the gadget
device release path completes and handles the add-error path symmetrically.
This prevents a dangling UDC pointer in `gadget_match_driver()` and related
device-lifetime paths. No CVE is asserted beyond the audited commit history.

## Build, ABI, KMI, and FBE

The following targets passed:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All ABI reports and the ABI error file are empty; the ABI diff exit code is
zero. B05 `.config` and `Module.symvers` are byte-identical to B04. The only
tracked runtime path is USB gadget core; the B04-to-B05 diff is empty across
fscrypt, F2FS, ext4, block/blk-crypto, device mapper, keyrings, SELinux, UFS,
SCSI, ICE, and crypto. FBE/storage runtime delta: **0 / PASS**.

| Artifact | SHA-256 |
|---|---|
| Image | `312cbbb85d4a757fb58f5310c565e5a2116f637fec2f88d63fd662e04d7999fc` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `f83732ec1e017191eb7c9b9ee8c96c7fe925dd5193dc71c67558f0a694c3819e` |
| vmlinux | `fe44a4357cef53bd9ec9e67fd8c99c9f84f488020c657f93e3cc65c23f49ade3` |

## Retained current-firmware module contract

| Gate | Result |
|---|---:|
| Physical module files checked | 1,020 |
| Active compatible module files | 984 |
| Dormant module files | 36 |
| Import/CRC edges matched | 57,216 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature/trust failures | 0 |
| Structural-provider failures | 0 |
| Static result | PASS |

The exact current-firmware contract remains:

- system_dlkm SHA-256: `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`;
- vendor_dlkm SHA-256: `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`;
- vendor_boot SHA-256: `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`;
- system `modules.load`: 82 entries;
- `wwan.ko`: entry 28;
- unexpected DLKM changes: 0.

## Boot-only payload

The packaging process first reproduced the physically qualified B04 boot
image byte-for-byte. B05 then used the same OOS 16.0.10.500 container, replaced
only the same-sized 39,889,408-byte kernel, and regenerated the outer footer
with the established unlocked-device development key and unchanged descriptor
values. The 4,096-byte header and 17,920-byte non-kernel tail match B04; the
embedded kernel matches B05 `Image`; AVB verification passes. No OEM private
key claim is made.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | current-firmware container, kernel replaced | `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b05-final/boot.img
```

The exact future boot-only qualification is in
`candidate-b05-physical-test-plan.md`. Normal USB compatibility and the exact
device-removal race remain separate outcomes; without runtime evidence of the
vulnerable interleaving, report **EXACT FIX TRIGGER NOT OBSERVED**.

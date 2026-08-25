# Candidate A: firmware-native r53 static validation

## Decision

**READY — FIRMWARE-NATIVE R53 SOURCE-KERNEL PHYSICAL TEST**

Candidate A changes only the kernel payload in an exact copy of the stock OOS
16.0.10.500 boot container. It does not provide, rebuild, or modify any DLKM,
`vendor_boot`, VBMeta, or slot metadata. No Candidate A partition has been
flashed.

## Source contract

| Field | Value |
|---|---|
| Branch | `experiment/oos1610500-ack-r53-base` |
| Firmware-native ACK tag | `android16-6.12-2025-06_r53` |
| Firmware-native ACK commit | `b2a876903b495c444a94b16f50d1463ffe953957` |
| Runtime source head | `99077ba8e1792b46d341d60f446b79d260f5f639` |
| Kernel release | `6.12.23-android16-5-o-g99077ba8e179-4k` |

The effective common source was reset to official r53. Two bounded integration
changes remain on top:

1. the OnePlus Kleaf wrapper omits the unavailable, non-runtime
   `vmlinux_oki` advertised output for this firmware-native GKI build; and
2. the exact OOS 16.0.10.500 stock GKI module certificate is added to the
   trusted keyring because `CONFIG_MODULE_SIG_PROTECT=y` and 103 retained stock
   system modules are signed by that certificate.

The stock certificate is public material only. No OEM private signing key is
available or claimed. Its DER sequence was deterministically found in both the
final `vmlinux` and `Image`.

## Build and kernel contract

The following targets completed successfully in one final build:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

| Artifact | SHA-256 |
|---|---|
| Image | `1696aafbe11880cb63ebd28ec852ee618fa461100ba53201ec7908727064217d` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `b9a7f87a8ee18c43f0e8e04781f604845524b5f9a154357a62d7a65351d2b74b` |
| vmlinux | `1f39046fe91740fdb03e71260228506911c00ae4288c6eba67247a77f5b14b0f` |

KMI symbol checking passed. All five ABI diff reports are empty (zero bytes).

Compared with the live stock `/proc/config.gz`, Candidate A has only two
semantic differences: the clang build-number string and the explicit stock
system-module trusted certificate. All FBE, fscrypt, block-inline-encryption,
dm-default-key, dm-crypt, F2FS, ext4, UFS-crypto, keys, and SELinux values are
stock-equivalent. See `candidate-a-stock-config-delta.txt`.

## Retained stock module contract

All module binaries come from exact stock OOS 16.0.10.500 images.

| Gate | Result |
|---|---:|
| Physical module files checked | 1,020 |
| Active compatible module files | 984 |
| Dormant module files | 36 |
| Import/CRC edges matched | 57,216 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Unknown/signature failures | 0 |
| Static result | PASS |

The 103 signed system modules identify the exact stock certificate and the
remaining 917 checked module files are unsigned retained vendor/vendor_boot
objects. Release-token-only vermagic differences are accepted only where the
ELF carries MODVERSION CRCs and its non-release vermagic suffix matches.

`oplus_bsp_file_read_record.ko`, newly present in this firmware, was retained.
Its 40 imports resolve with matching CRCs in both normal vendor_dlkm and
recovery vendor_boot inventories.

The firmware-native system load policy is untouched:

- `modules.load`: 82 entries
- `wwan.ko`: entry 28

## Stock boot-container derivation

Candidate A was derived from stock `boot_a` SHA-256
`b811796df7ce5fa3b6da07fd00b65c2f9d7bbe1c38939f91f2fc48cc12ada46d`.
The stock kernel payload and final Candidate A Image are both exactly
39,889,408 bytes. The following remained byte-identical to stock before AVB
re-signing:

- the complete 4096-byte boot header;
- the zero-byte ramdisk, empty cmdline, absent bootconfig, and absent DTB;
- the 17,920-byte non-kernel tail in the original boot image;
- original image size, partition size, salt, partition name, fingerprint
  property, OS-version property, and security-patch property.

The OEM AVB private key is unavailable. The unlocked-device GKI development
key was used without pretending to reproduce the OEM signature. `avbtool
verify_image` passed, and unpacking proved the embedded kernel is byte-identical
to the final built Image.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | stock container, kernel replaced | `30195cbfe97e3cd831b4e84ed575c68cb5d079f18bbed1e6ee84a91a32cdfb70` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

## Physical isolation contract

The later physical test must start from a healthy stock slot `_a`, use the
verified stock rollback boot, perform the hardened TWRP backup and dry run,
and write **boot only**. The stock system_dlkm and vendor_dlkm read-back hashes
must be checked before booting Android. Existing encrypted userdata and
metadata remain untouched.

Candidate A passes only if Android initializes existing user0 and boots with
the expected r53 source-built kernel while both stock DLKMs remain exact. If it
returns to recovery with `init_user0_failed`, restore the exact stock boot and
continue source/build-contract analysis. Candidate B is prohibited until this
physical test passes.

# Candidate B01: Binder lifetime hardening static validation

## Decision

**READY — CANDIDATE B01 PHYSICAL TEST**

Candidate B01 adds one bounded runtime change to the physically qualified
firmware-native Candidate A kernel. It does not build, replace, or modify
`system_dlkm`, `vendor_dlkm`, `vendor_boot`, VBMeta, or slot metadata. No B01
payload has been flashed.

## Source and change boundary

| Field | Value |
|---|---|
| Qualified Candidate A checkpoint | `e0e55a0625a938c9aa8c0dbaef0e6abff9664184` |
| Candidate A runtime source | `99077ba8e1792b46d341d60f446b79d260f5f639` |
| B01 branch | `experiment/oos1610500-custom-r53-b01` |
| B01 runtime source | `35f7cb764f0ade56b94df3b16a40443cc98be4c1` |
| Kernel release | `6.12.23-android16-5-o-g35f7cb764f0a-4k` |

The only runtime delta is `drivers/android/binder.c`, taken from controlled
project commit `5c4058a84655b76abb22cf255dea5761788674d3`. Its provenance is ACK commits
`1ac5be05b2854ba2329c0e52a67edd80b3ab4352` and
`2df1e89cb5aab41eee1ca77f03785d7a4809ad60`, corresponding to upstream
`114a116aaa5f0295376cdf12da743c5bce3b20ce` and
`f223d27a546c1e1f48d38fd67760e78f068fe8c4`.

`binder_free_transaction()` now pins the transaction target thread while it
uses the associated target process during teardown. This is a Binder
memory-lifetime hardening fix in the built-in Android IPC implementation. It
changes no configuration, public export, module provider, device tree, FBE,
fscrypt, block-crypto, device-mapper, keyring, or storage path. Disassembly of
the final `vmlinux` confirms the rebuilt function contains the new lock and
reference-control flow.

## Build and ABI/KMI contract

The following targets passed in one build:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

| Artifact | SHA-256 |
|---|---|
| Image | `8ffb6488e5683ecbb7d1bcb06a64820f49e9229add85cbcc566db121ad0c11ae` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `4ac0314f159a11bf6884f6a3da44a996aa895e17fabe6c8252a0a4a11c97e603` |
| vmlinux | `c6593df367496704668c78812e7e85e923c50176f5c876036bc7f9549cd6f689` |

The `.config` and `Module.symvers` hashes are byte-identical to Candidate A.
All five ABI reports and the ABI error file are empty. KMI strict-mode and KMI
symbol-list checks passed. No export or MODVERSION contract changed.

## FBE and existing-user0 contract

Candidate A's physically passing configuration is unchanged. Required
facilities remain enabled, including fscrypt, inline encryption and fallback,
dm-default-key, dm-crypt, kernel keys, UFS crypto, and SELinux. The exact stock
GKI module certificate remains embedded in both `Image` and `vmlinux`.

The B01 source delta has no path under F2FS, ext4, fscrypt, block crypto,
device mapper, key management, UFS, SCSI, ICE, or Android storage hooks.
Static result: **PASS**. Physical validation must still prove that existing
encrypted user0 reaches `RUNNING_UNLOCKED`.

## Exact retained stock module contract

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
| Static result | PASS |

The stock firmware-native load contract remains untouched:

- `modules.load`: 82 entries
- `wwan.ko`: entry 28
- system_dlkm SHA-256:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- vendor_dlkm SHA-256:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`

Unexpected DLKM changes: **0**.

## Boot-only candidate

The B01 Image is exactly the same size as Candidate A and the stock kernel
payload. The candidate was derived from the OOS 16.0.10.500 stock `boot_a`
container. Before AVB development re-signing, the complete boot header and
the non-kernel tail were byte-identical to stock. Unpacking proves the embedded
kernel is byte-identical to the final built Image.

`avbtool verify_image` passes with the established unlocked-device GKI
development key. The OEM AVB private key remains unavailable and is not
claimed.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | stock current-firmware container, kernel replaced | `2646a4d773ac6360cf981c4148fd37b128e8f0cd53abd07418a6807641e9d091` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b01-final/boot.img
```

## Required physical isolation

The later physical test must use the established backup, dry-run, exact-hash,
read-back, rollback, and one-variable gates. Write **boot_a only**. Verify both
stock DLKM block hashes before booting and do not write them.

Acceptance requires two Android boots, existing user0 `RUNNING_UNLOCKED`, no
`init_user0_failed`, 6 GHz WPA3, cellular registration and RMNET dual stack,
routes and IP/DNS, Bluetooth basic recovery, NFC service health, deep idle and
wake, Android framework/app-launch stress, and a clean kernel/module error
scan.

Rollback is Candidate A boot SHA-256
`30195cbfe97e3cd831b4e84ed575c68cb5d079f18bbed1e6ee84a91a32cdfb70`
with the same exact stock DLKMs.

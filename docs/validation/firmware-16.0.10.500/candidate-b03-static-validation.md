# Candidate B03: eventpoll lifetime hardening static validation

## Decision

**READY — CANDIDATE B03 PHYSICAL TEST**

Candidate B03 cumulatively contains the physically qualified B01 Binder
lifetime hardening and B02 BPF per-CPU map bounds hardening plus one new
coherent eventpoll lifetime series. No ACK advancement, configuration change,
DLKM adaptation, built-in conversion, device-tree change, or unrelated cleanup
is included. The B03 payload has not been flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B02 parent | `635709f3b14eaef8778abfbe92b8fbec3ed7e02e` |
| B03 branch | `experiment/oos1610500-custom-r53-b03` |
| B03 runtime source | `c3b68584dbb4638abe27a69b7f421826625d4a53` |
| Kernel release | `6.12.23-android16-5-o-gc3b68584dbb4-4k` |
| Intended B03 runtime path | `kernel_platform/common/fs/eventpoll.c` |
| Unexpected runtime groups | 0 |

The qualified B02 HEAD is an ancestor of B03. B01 and B02 remain present and
are neither duplicated nor modified.

## Selected value and provenance

The r53 implementation is pre-fix and `CONFIG_EPOLL=y`. The selected group is
the ordered controlled-project series:

- `047ce9b3fc181542ffbd9c2292e8902a0b5b17e6`, carrying ACK commits
  `f01453f20745`, `2d6bc6abd42a`, `64cea30f9b93`, `3dff0800fae6`,
  `3de54f8c502a`, `690d6106eec6`, and `500d67e5110a`;
- `5b4c2b4675c7d6f457984d12ede9bace6ca7337a`, carrying ACK commit
  `215bb7dbd9d4` and upstream `07712db80857`.

The first part pins the watched file while `ep_remove()` tears down the item
and separates removal from the file-release path. The follow-up adds an RCU
head to `struct eventpoll` and defers its final free past RCU readers. Together
they prevent file, epitem, and eventpoll lifetime use-after-free or wrong-slab
free conditions. The audited commits establish memory-lifetime hardening; this
report does not assign a CVE not present in their source provenance.

## Build, ABI, and KMI

The following targets passed under B02's exact Zstd-DWARF setting:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All five ABI reports and the ABI error file are empty. KMI strict-mode and KMI
symbol-list violation checks passed. B03 `.config` and `Module.symvers` are
byte-identical to B02, so there are zero configuration or exported-CRC changes.

| Artifact | SHA-256 |
|---|---|
| Image | `8ba5dbb28e2688ed7a518358f14b10316d358b2e00001c28ef29f23e7ee8902a` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `38b5f8934ee3efdc221c24542f7c93ae9c39f4909f9904e5d91c456a348b6ff4` |
| vmlinux | `be39b2001d6c74c3f5468a619487f39bb18d58ea2d52cf1fdda5ce6951a47a4e` |

## FBE and existing-user0 contract

The source diff is empty across fscrypt, F2FS, ext4, block and blk-crypto,
device mapper, keyrings, SELinux, UFS, SCSI, ICE-sensitive paths, and crypto.
There is no encryption or storage configuration delta. Static FBE/storage
runtime delta relative to physically qualified B02: **0 / PASS**.

Physical qualification must still prove existing user0 reaches
`RUNNING_UNLOCKED` on two boots and that `init_user0_failed` is not observed.

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

The exact current-firmware payload and load-policy contract remains:

- system_dlkm SHA-256:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- vendor_dlkm SHA-256:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- vendor_boot SHA-256:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`
- system `modules.load`: 82 entries
- `wwan.ko`: entry 28
- unexpected DLKM changes: 0

## Boot-only payload

The candidate uses the physically proven OOS 16.0.10.500 stock-container
method. Re-applying that method to the B02 boot reproduced B02's exact
`dd63f38c...` SHA-256 byte-for-byte. In B03, the 4,096-byte header and
17,920-byte non-kernel tail remain byte-identical to B02/current firmware; only
the 39,889,408-byte kernel payload changes. Unpacking proves that embedded
kernel is byte-identical to the accepted B03 Image.

The outer AVB hash footer verifies with the established unlocked-device GKI
development key. Partition size, original-image size, salt, partition name,
fingerprint property, OS-version property, and security-patch property are
preserved. The OEM private key remains unavailable and is not claimed.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | stock current-firmware container, kernel replaced | `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b03-final/boot.img
```

## Physical-test requirement

The exact plan is recorded in `candidate-b03-physical-test-plan.md`. Write
`boot_a` only after the established preflight, verified backup, dry run,
candidate-hash, and full read-back gates. Roll back to exact B02 boot SHA-256
`dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07`
on the first critical failure.

Normal-runtime compatibility and exact race coverage are separate outcomes.
If safe epoll churn does not prove the precise close/removal/RCU race occurred,
record **EXACT FIX TRIGGER NOT OBSERVED**.

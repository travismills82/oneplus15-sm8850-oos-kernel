# Candidate B04: AF_PACKET fanout lifetime hardening static validation

## Decision

**READY — CANDIDATE B04 PHYSICAL TEST**

Candidate B04 cumulatively contains the physically qualified Candidate A,
B01 Binder lifetime hardening, B02 BPF per-CPU map bounds hardening, and B03
eventpoll lifetime hardening plus one new AF_PACKET fix. It does not advance
ACK, change configuration, modify a module provider, adapt a DLKM, convert a
module to a built-in, alter device tree, or require firmware or userspace.
The B04 payload has not been flashed.

## Source identity

| Field | Value |
|---|---|
| Qualified B03 parent | `8f3ee08db58afff44881694676edd678d0baffe1` |
| B03 tested runtime source | `c3b68584dbb4638abe27a69b7f421826625d4a53` |
| B03 tested boot | `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0` |
| B04 branch | `experiment/oos1610500-custom-r53-b04` |
| B04 runtime source | `2f2631b951ced2ef05a4a9643610954b26736bcd` |
| Kernel release | `6.12.23-android16-5-o-g2f2631b951ce-4k` |
| Intended B04 runtime path | `kernel_platform/common/net/packet/af_packet.c` |
| Unexpected runtime groups | 0 |

The qualified B03 freeze commit is an ancestor of B04. The complete tracked
runtime diff is one insertion in `packet_release()`.

## Selected value and provenance

The exact firmware-native r53 implementation is pre-fix and
`CONFIG_PACKET=y`. B04 carries ACK commit
`75fe6db23705a1d55160081f7b37db9665b1880b`, whose upstream source is
`42156f93d123436f2a27c468f18c966b7e5db796`.

`packet_release()` previously removed the protocol hook while holding
`po->bind_lock` but left `po->num` nonzero. A `NETDEV_UP` notifier that had
already found the socket could then re-register the closing socket in its
fanout group's `arr[]` without acquiring the corresponding fanout socket
reference. B04 performs `WRITE_ONCE(po->num, 0)` under the same lock, making
the notifier reject the closing socket and preventing the dangling fanout
pointer.

This is real, compiled memory-lifetime hardening. The audited ACK and upstream
provenance does not assign a new CVE to this commit, so this report does not
repeat the unverified CVE label from the historical controlled commit.

## Build, ABI, and KMI

The following targets passed with the exact B03 Zstd-DWARF setting:

```text
//common:kernel_aarch64
//common:kernel_aarch64_abi
//common:kernel_aarch64_abi_kmi_symbol_checks
//common:kernel_aarch64_abi_diff
```

All five ABI reports and the ABI error file are empty; the ABI diff exit-code
file contains `0`. KMI strict mode and symbol-list checks passed. B04 `.config`
and `Module.symvers` match the B03 qualified manifest hashes byte-for-byte, so
there is no semantic configuration or exported-CRC delta.

| Artifact | SHA-256 |
|---|---|
| Image | `674b906f1eb3989ccc7bb452f047f76a3d8fbfd0aa991394f1824663bae77888` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `3892f6089c419f3557610a65af35995778286e6e44638f6a48dd67ca94ae8cb4` |
| vmlinux | `dc8937db7c044a5c6c1dc83319abddc6a14ef82ed6f4778c741e17a8f225b343` |

## FBE and existing-user0 contract

The B03-to-B04 source diff is empty across fscrypt, F2FS, ext4, block and
blk-crypto, device mapper, keyrings, SELinux, UFS, SCSI, ICE-sensitive paths,
and crypto. The full config is byte-identical. Static FBE/storage runtime
delta: **0 / PASS**.

Physical qualification must still prove that existing encrypted user0 reaches
`RUNNING_UNLOCKED` twice and that `init_user0_failed` is not observed.

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

The packaging method first reproduced the exact physically tested B03 boot
SHA-256 byte-for-byte. B04 then used that same current-firmware container,
removed only its outer AVB footer, replaced the same-sized 39,889,408-byte
kernel payload, and regenerated the footer with the already established
unlocked-device GKI development key and preserved OOS 16.0.10.500 descriptor
values.

The 4,096-byte header and 17,920-byte non-kernel tail are byte-identical to
B03. Unpacking proves the embedded kernel equals B04 `Image`. Outer AVB
verification passes. Image size, original-image size, salt, partition name,
fingerprint, OS version, and security-patch properties are preserved. No OEM
private-key claim is made.

| Payload | Mode | SHA-256 |
|---|---|---|
| boot | current-firmware container, kernel replaced | `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3` |
| system_dlkm | exact stock, unchanged | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| vendor_dlkm | exact stock, unchanged | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| vendor_boot | exact stock, unchanged | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Candidate path:

```text
/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b04-final/boot.img
```

## Physical-test requirement

The exact future plan is recorded in `candidate-b04-physical-test-plan.md`.
Only `boot_a` may be changed after the established preflight, verified backup,
dry-run, candidate-hash, and complete read-back gates. Roll back to exact B03
boot `0b065aa6...` on the first critical failure.

Normal compatibility and exact race coverage remain separate outcomes. Unless
existing logs or tracepoints prove both the fanout close and concurrent
`NETDEV_UP` re-registration condition occurred, report **EXACT FIX TRIGGER NOT
OBSERVED**.

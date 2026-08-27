# New current-firmware ACK 6.12.24 versus the former failed candidate

## Scope and conclusion

This comparison separates observed facts from root-cause inference. The old
ACK 6.12.24 boot failed on OOS 16.0.10.500 with `init_user0_failed`; the new
candidate has passed static validation only and has not been flashed. No
single difference below is declared the root cause without a controlled
physical result.

## Lineage and effective source

| Property | Former failed candidate | New current-firmware candidate |
|---|---|---|
| Runtime source | `a7f2fd6d686f38d448e8a276efe1aea7c2b9013f` | `7bd04d49c555d0764568b631bd1ba8990235c2ae` |
| Kernel release | `6.12.24-android16-5-o-ga7f2fd6d686f-4k` | `6.12.24-android16-5-o-g0348b41c16e5-4k` (release-stamped documentation head) |
| Effective firmware base | former OOS 16.0.9.400 controlled source/config lineage | OOS 16.0.10.500 firmware-native r53, then physically qualified B01-B08 |
| ACK migration | former controlled 6.12.24 integration | freshly audited Linux 6.12.23 to 6.12.24 range on the final custom-r53 checkpoint |

The old runtime commit remains a historical Git ancestor because Candidate A
realigned the effective common source in later commits rather than rewriting
history. It was not used as the effective source base for this candidate. The
new branch starts at `feature/oos1610500-custom-r53-qualified`, whose metadata
head is `6a351339c02dd72b7f797b54aff3563afea35c34` and whose physically tested
runtime head is `338c09465853ddbccde4861e14f8c8fa2f24342e`.

The new migration classifies all 394 official stable-range commits: 374 apply,
18 are functionally already present in r53, and two require reviewed Android
KMI integrations. The B01-B08 path intersection is zero. The resulting
runtime series changes 363 paths, all under `kernel_platform/common/`, with no
new project feature, DLKM source, device tree, or vendor source included.

## Configuration

The archived stock-16.0.10.500-to-old-candidate comparison contains 1,154
reported lines. Among its material differences were numerous module-to-built-in
conversions, removal of `MODULE_SIG_PROTECT`/protected-module policy, disabled
unused-KSYMS trimming, different debug-info compression, additional filesystems
and networking features, and a global workqueue-policy change.

The new candidate has zero semantic configuration differences from physically
qualified B08. Its only textual `.config` change is the generated 6.12.24
version banner. It retains the firmware-native stock-module certificate and
module-validation configuration.

## FBE and storage

The new point-release delta does not touch fscrypt, blk-crypto,
dm-default-key, dm-crypt, keyrings, F2FS, UFS, or ICE source. Its reviewed
storage-adjacent changes are ext4 correctness fixes, dm-verity read/suspend
correctness, two disabled dm targets, and disabled non-UFS SCSI drivers. See
`../ack-modernization/oos1610500-ack-6.12.24-fbe-storage-audit.md`.

This narrows the new candidate's static storage delta but does not prove the
old failure's exact cause. The old lineage/config/build-container interaction
remains the correct broad classification until the new candidate is tested.

## Boot container

| Property | Former failed boot | New boot |
|---|---:|---:|
| SHA-256 | `3ceb46491d029586af1a6dc494b5baf4ddb973ad0c065c0960e4ed307d9d40b9` | `9e2a7cebe4861d077dc38486b1c8841302ca646880d6bde55dfecb238151d8ad` |
| Kernel payload bytes | 47,962,624 | 39,954,944 |
| Unpadded AVB image bytes | 47,984,640 | 39,976,960 |
| Header | v4, no ramdisk/cmdline | v4, no ramdisk/cmdline |
| Fingerprint property | `.../1780323679878:user/release-keys` | `.../1785498538399:user/release-keys` |
| Security-patch property | `2026-06-01` | `2026-08-01` |
| AVB salt | `e09832ccd5adbd33a7e32c3afd49f5b3400d325eb5d2fa07c1be902cd66efeb0` | `a59ca843234f48c50cbf412a03e8b55e` |

The new packager was first proven by reproducing the physically tested B08
boot byte-for-byte. The candidate then replaces only the kernel payload,
updates the v4 kernel-size field, preserves the exact 16 KiB firmware-native
GKI signature tail, and retains the B08/current-firmware AVB properties and
partition geometry. Both boots use the established unlocked-device
development AVB key; neither is represented as OEM-signed.

## Static module result

The new Image passes the exact stock current-firmware universe: 1,020 modules,
57,216 import/CRC edges, zero unresolved imports, zero CRC mismatches, zero
protected-export failures, zero signature failures, and zero structural
provider failures. The former candidate also passed a static module audit,
which is why this result does not replace the required existing-user0 physical
test.

# OOS 16.0.10.500 `init_user0_failed` evidence analysis

## Scope

This record analyzes the retained evidence from the failed boot-only isolation
test on OnePlus 15 CPH2747 / Canoe. The tested combination was:

- boot: ACK 6.12.24, SHA-256
  `3ceb46491d029586af1a6dc494b5baf4ddb973ad0c065c0960e4ed307d9d40b9`
- system_dlkm: exact OOS 16.0.10.500 stock, SHA-256
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- vendor_dlkm: exact OOS 16.0.10.500 stock, SHA-256
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`

No partition was written during this analysis. No userdata or metadata wipe is
permitted as a compatibility remedy.

## Retained evidence

The captured TWRP recovery log records Android's recovery request as:

```text
boot command: boot-recovery
Android Rescue Party trigger!
'--reason=init_user0_failed'
```

This proves that the first physical incompatibility boundary is the
boot/kernel/early-user0 path. It does not identify the failing syscall, kernel
facility, or errno returned to vold.

The recovery capture contains no retained Android vold, `vdc cryptfs
init_user0`, fs_mgr, or first-stage-init transcript. Searches found no
`ENOKEY`, `EKEYREJECTED`, `EINVAL`, `EOPNOTSUPP`, `EPERM`, `EACCES`,
device-mapper error, fscrypt error, or inline-crypto error attributable to the
failed Android boot. The recovery pstore directory was empty, and no usable
`last_kmsg` was retained.

## Important non-evidence

Later in the same log, after TWRP itself starts, TWRP reports:

```text
I:Unable to decrypt metadata encryption
I:FBE setup failed. Trying FDE...
```

That is TWRP's independent recovery decryption attempt. It occurs after the
Android rescue reason is consumed and must not be presented as the errno or
root cause from Android's earlier `vdc cryptfs init_user0` operation.

TWRP did recover the firmware's storage contract from fstab:

```text
contents:  aes-256-xts
filenames: aes-256-cts:v2+inlinecrypt_optimized+wrappedkey_v0
metadata:  wrappedkey_v0
data fs:   f2fs with inline crypto
```

This confirms which kernel facilities must remain available, but it does not
show which facility failed.

## Configuration comparison

The live stock configuration and the failed controlled configuration agree on
the core prerequisites under investigation:

- `CONFIG_FS_ENCRYPTION=y`
- `CONFIG_FS_ENCRYPTION_INLINE_CRYPT=y`
- `CONFIG_BLK_INLINE_ENCRYPTION=y`
- `CONFIG_BLK_INLINE_ENCRYPTION_FALLBACK=y`
- `CONFIG_DM_DEFAULT_KEY=y`
- `CONFIG_DM_CRYPT=y`
- `CONFIG_KEYS=y`
- F2FS and ext4 encryption support
- UFS crypto support
- SELinux support

Therefore the failure is not explained by one of those headline symbols being
disabled. The complete comparison is in
`oos1610500-stock-vs-controlled-config.tsv`; differences still require source,
build, and trust-contract isolation rather than blind symbol enablement.

## ACK provenance result

The OOS 16.0.10.500 stock kernel identity maps to official ACK tag
`android16-6.12-2025-06_r53`, commit
`b2a876903b495c444a94b16f50d1463ffe953957`.

The old project manifest also named that ACK commit, but its effective common
tree contained project, Qualcomm, and OnePlus modifications. Comparing that
effective tree with exact r53 found 107 changed common-kernel paths. No direct
path delta occurs under `fs/crypto`, `block/blk-crypto*`,
`drivers/md/dm-default-key*`, `drivers/md/dm-crypt*`, or `security/keys`; eight
F2FS paths do differ. This rules out the simple theory that the old manifest
merely omitted a later r53 ACK roll-up, while still leaving project-tree and
build-contract differences to isolate.

## Boot-container comparison

Both stock and failed images use boot header v4, 4096-byte alignment, a raw
ARM64 Image, no boot ramdisk, no boot cmdline, no bootconfig, and no embedded
DTB. The failed image differed in kernel bytes/size, AVB development-key
identity, fingerprint property, and security-patch property.

Candidate A therefore uses the exact stock OOS 16.0.10.500 boot container as
its template and changes only the kernel payload. Stock header fields,
non-kernel layout, salt, descriptor properties, and partition size are
preserved. The OEM AVB private key is unavailable; the unlocked-device
development key is used explicitly and is not represented as an OEM
signature.

## Root-cause status

**The precise `vdc`/vold failure and errno were not retained, so a complete
root cause is not yet proven.**

The evidence supports a controlled next experiment: a source-built,
firmware-native r53 kernel in the stock boot container with both stock DLKM
images unchanged. Candidate A tests whether the firmware-native common source,
configuration, module-trust, and boot-container contract can reproduce a
bootable kernel before any project customization or ACK 6.12.24 delta is
introduced.

## Candidate A physical result

Candidate A subsequently passed two clean Android boots. Existing user 0
transitioned from the expected pre-credential `RUNNING_LOCKED` state to
`RUNNING_UNLOCKED`, and credential-encrypted storage was readable. Exact stock
OOS 16.0.10.500 system_dlkm and vendor_dlkm hashes were retained. The available
boot logs contained no new FBE, fscrypt, dm-default-key, inline-crypto, module,
panic, or IOMMU/SMMU failure.

The evidence now excludes these broad explanations:

- stock OOS 16.0.10.500 DLKM incompatibility by itself;
- source-built kernel incompatibility by itself; and
- the stock OOS 16.0.10.500 boot-container contract by itself.

The first-bad boundary remains between the passing firmware-native r53 source
kernel and the former controlled ACK 6.12.24 boot. Still plausible are the old
effective common-tree lineage, project customizations, configuration delta,
ACK 6.12.24 integration, build/release contract, or an interaction among
those. No exact individual change is identified yet.

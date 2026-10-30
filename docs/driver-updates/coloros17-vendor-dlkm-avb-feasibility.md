# ColorOS 17.0.0.102 vendor_dlkm authenticated deployment feasibility

Audit date: 2026-10-30. Device: OnePlus 15 / SM8850 / CPH2745IN.
Branch: `experiment/coloros17-sm8850-dlkm-updates`.
Source HEAD preserved: `0c4d5b23ab7ca52462fd40cb1c9ecb02438a0fe0`.

## Decision

**BLOCKED — no supported authenticated replacement-parent signing or custom-key
enrollment method has been established for this exact OnePlus 15 firmware.**

The blocker is not image-generation capability or the RMNET module ABI.
Stock top-level `vbmeta` directly authenticates `vendor_dlkm`'s hashtree root.
A changed filesystem needs a changed, accepted parent descriptor. A newly
signed local footer, an unrelated module-signing certificate, or a working
custom boot image does not supply that authority.

Custom-key support remains **UNKNOWN**, not proven unsupported. This report
does not claim that deployment is impossible under every future supported
OEM/developer arrangement. It establishes what is and is not justified by
the available exact images, archived observations and primary documentation.

No kernel rebuild, source/config repair, key generation/enrollment, new signed
image, device write/reboot, verification disablement, rollback-index change,
commit, push or publication was performed. Existing RMNET commits, their test
artifacts and the qualified boot-only release are preserved.

## 1. Exact inputs and offline evidence

Stock files are from `/home/travis/Downloads/CPH2745IN_17.0.0.102/`.
These are full firmware images, not the old filesystem-only `*-verity.img`
views. File hashes are not interchangeable with hashes of a padded physical
partition or of the signed VBMeta payload within it.

| Input | Bytes | SHA256 |
|---|---:|---|
| `other/vbmeta.img` | 12288 | `5338ad8b14d98c41e0be8f10a8ad4fc48218dc61f3fcba362fc2f2167c171cc9` |
| `other/vbmeta_vendor.img` | 4096 | `e7a4f5e4ef68309834e6f944cea5da1bd7f1004dc7a57950baf03cdc3117be0d` |
| `other/vbmeta_system.img` | 4096 | `d5dd532062c83e4d442a58342561cd0215ac6be7710baf8479a1c99a145d0cb3` |
| `vendor/vendor_dlkm.img` | 154849280 | `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47` |
| `system/system_dlkm.img` | 14512128 | `3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a` |
| `critical/abl.img` | 278528 | `44afb9b19e1d00bd929cf55672efb8100644c7e0d535d8137ed4281bfa113438` |

Tool: manifest prebuilt
`kernel_platform/prebuilts/kernel-build-tools/linux-x86/bin/avbtool`,
version1.3.0, SHA256
`6fdee0f2ded47f985fb6fc3c2c2abeaae5c0b4df5707eae9a4f22e3ca7fad4f2`.

New private evidence directory:

`/home/travis/Android/kernel-test-backups/coloros17-vendor-dlkm-avb-feasibility-20261030/readiness/`.

`stock-avb-topology-and-verification.json` records parsed headers/descriptors,
key identities, exact command lines, expected/actual exits and hashes before
and after. The image view contains symlinks to stock inputs, not modified
images. Only public keys were extracted; no private key was read or created.
All26 command outcomes match expectations. The41-file evidence manifest
`evidence-sha256.txt` verifies; its SHA256 is
`ab873a8dd70d0281d30c5b06031d5e3dcbc8e3ef6c0fcb12206d3197d1de3cd1`.

## 2. Actual stock chain topology

This topology is decoded from these images, not a generic Android layout.
`K0`/`K1` below identify embedded/pinned public keys; the device's ultimate
hardware/OEM trust decision is not independently proven by an image dump.
This is not a complete Qualcomm secure-boot/fuse audit or a claim about every
other OEM partition outside the inspected VBMeta set.

```text
device bootloader trust policy (exact custom-key support unverified)
  |
  +-- vbmeta: SHA256_RSA4096 / K0 / rollback location0, index0 / flags0
        +-- hash: init_boot
        +-- hash: vendor_boot
        +-- hashtree: vendor_dlkm       <-- direct authenticated descriptor
        +-- hashtree: system_dlkm       <-- direct authenticated descriptor
        +-- chain: boot, K0, rollback location4
        +-- chain: dtbo, K0, rollback location3
        +-- chain: recovery, K0, rollback location1
        +-- chain: vbmeta_system, K1, rollback location2
        |     +-- hash: pvmfw
        |     +-- hashtree: product, system, system_ext
        +-- chain: vbmeta_vendor, K1, rollback location5
              +-- hashtree: vendor
```

There is **no vendor_dlkm chain descriptor** in stock top-level vbmeta, and
neither `vbmeta_vendor` nor `vbmeta_system` owns its descriptor.
Changing `vbmeta_vendor` alone does not update the existing parent pin for
vendor_dlkm. Adding another vbmeta file cannot establish a new trusted edge
without an accepted change to the parent contract.

### Public keys and rollback requirements

| Key | RSA bits | avbtool SHA1 identifier | SHA256 of raw AVB public key |
|---|---:|---|---|
| K0, stock root and boot/dtbo/recovery chain pins | 4096 | `a568677dbd61e0aa97ca6f3183dc28f505d1b83c` | `8d897f62492ea617f777bad41a5711ab621fcac1efc1865b890328ee8c3853bb` |
| K1, stock system/vendor vbmeta chain pins | 2048 | `42e345c9946cbb9126e0fb590b67ca71e4e66667` | `1aef65cb06c0146a2071ed14eeb05f09a296bef1e20d0a1e81ef813bed0599c2` |

The SHA1 values are key identifiers printed by avbtool, not the signature
algorithm. Stock metadata uses SHA256_RSA4096 or SHA256_RSA2048.

| VBMeta object | Signing algorithm | Effective rollback location | Image rollback index |
|---|---|---:|---:|
| top-level vbmeta | SHA256_RSA4096 / K0 | 0 | 0 |
| boot | SHA256_RSA4096 / K0 | 4, from parent | 0 |
| dtbo | SHA256_RSA4096 / K0 | 3, from parent | 0 |
| recovery | SHA256_RSA4096 / K0 | 1, from parent | 1 |
| vbmeta_system | SHA256_RSA2048 / K1 | 2, from parent | 1790812800 |
| vbmeta_vendor | SHA256_RSA2048 / K1 | 5, from parent | 1790812800 |

The latter index corresponds to 2026-10-01. Child header location0 fields do
not override their parent's chain locations2/5. Local DLKM unsigned metadata
has index/location0 but supplies no independent signing authority or new
delegation. Actual stored rollback indexes and any additional OEM antirollback
policy are unavailable offline. Do not reset or lower indexes to obtain boot.

All inspected stock VBMeta/header/descriptor flags are0; this is not evidence
that the currently absent device is locked or that a new root would be trusted.

### Exact vendor_dlkm verification metadata

Both top-level vbmeta and the local footer contain byte-identical serialized
vendor_dlkm hashtree descriptors. The analogous system_dlkm descriptors also
match. The local footer is metadata/integrity storage, **not a signed chain**.
The serialized vendor descriptor is256 bytes, SHA256
`37a29c35ff29db049c3a1c88017b4160c256035dc33fb8c594bbc0306166b6f6`.

| Field | vendor_dlkm | system_dlkm, unchanged control |
|---|---|---|
| Filesystem | ext4 | EROFS |
| Full image size | 154849280 | 14512128 |
| dm-verity version | 1 | 1 |
| Data size / tree offset | 152317952 | 14270464 |
| Tree size | 1208320 | 118784 |
| Data / hash block sizes | 4096 / 4096 | 4096 / 4096 |
| Hash algorithm | sha256 | sha256 |
| FEC roots | 2 | 2 |
| FEC offset / size | 153526272 / 1220608 | 14389248 / 114688 |
| Flags | 0 | 0 |
| Local footer version | 1.0 | 1.0 |
| Local VBMeta algorithm | NONE | NONE |
| Local VBMeta offset / size | 154746880 / 768 | 14503936 / 512 |

vendor_dlkm salt:
`87945e0bf09af70da8e6c38180950738c060aba9b1ef8f25f071e7e39375fee3`.
Root:
`1374ff0726349e3ca91b2999a786c12af12530aff39a836c58dae68839f1669c`.

system_dlkm salt:
`9e8e1b3e6af06f0c2f2cc7448b07763b33a8edd650608374fe63921d51bcb7bf`.
Root:
`ab2a8566a22027e27765f4390452b7892903efcde22f407682fb5c171bdc1335`.

Top-level vbmeta also retains the exact boot/init_boot/vendor_boot OS,
fingerprint and October security-patch properties. Duplicate existing boot
security-patch properties are not silently removed. A future accepted package
must preserve unrelated descriptors, properties, chain keys, flags and rollback
locations/values; this task changes none of them.

## 3. Exact mount policy and archived device evidence

Stock vendor_boot's `first_stage_ramdisk/fstab.qcom` rows47–48 specify ext4
and EROFS alternatives for vendor_dlkm, with
`wait,slotselect,avb=vbmeta,logical,first_stage_mount`. They do not contain
`avb_keys`. GSI key paths occur on system/system_dlkm entries, not vendor_dlkm.
Their presence therefore does not authorize a new vendor_dlkm root or signer.
The exact archived fstab SHA256 is
`0fd34410c73a1b6f4b8b0b268d1ee9fb047bfe7f2d0bebd5c72082b07aac4365`.

The archived `.102` baseline contains a 65536-byte `images/vbmeta_b.img`,
SHA256 `750918fafedd2ffbaaceee5830f2f8a77ed1b5c8d6e149f05739dadc39c93918`.
It differs in whole-file hash/length from the downloaded12288-byte image, but
both contain the same signed payload at bytes `[0,8448)`, SHA256
`0e9794f46784959d9dd0ada259433ea4182243860815b465afd49f5ee39671aa`.
Everything after that payload is zero in both. This is padding, not evidence
of another signer or disabled verification.

Archived properties record vendor_dlkm's sha256 root as exactly the stock
root above, `partition.vendor_dlkm.verified=2` and enforcing verity mode;
the recorded mount is ext4 read-only on dm-28. These are useful observations,
not a fresh authenticated measurement of the device's bootloader policy.
The numeric property is not treated as a generic PASS enum or proof of custom
image acceptance; its reference-code interpretation is discussed below.

In pinned AOSP17 reference code, init publishes that property using the
verity mode after checking the dm target; mode2 means restart, not verification
disabled. The observed value/root is consistent with that implementation,
but the shipping OEM publisher is not source-matched here.
[init publisher](https://android.googlesource.com/platform/system/core/+/545d2487e38192a2ce25040897ced877cf6b4f53/init/builtins.cpp),
[mode definitions](https://android.googlesource.com/platform/system/fs/fs_mgr/+/157112fcbaa00b6e84c63146cfde249c56cd652c/include/fs_mgr.h),
[dm queries and mode handling](https://android.googlesource.com/platform/system/fs/fs_mgr/+/157112fcbaa00b6e84c63146cfde249c56cd652c/fs_mgr.cpp).

That reference's first-stage code distinguishes per-entry standalone keys
from its common AVB handle. Unlocked error allowances do not automatically
disable installation of a hashtree using a descriptor from loaded VBMeta.
These are reference semantics, not proof of exact OnePlus control flow.
[first-stage mount](https://android.googlesource.com/platform/system/core/+/545d2487e38192a2ce25040897ced877cf6b4f53/init/first_stage_mount.cpp),
[AVB handle](https://android.googlesource.com/platform/system/fs/fs_mgr/+/157112fcbaa00b6e84c63146cfde249c56cd652c/libfs_avb/fs_avb.cpp),
[descriptor/table setup](https://android.googlesource.com/platform/system/fs/fs_mgr/+/157112fcbaa00b6e84c63146cfde249c56cd652c/libfs_avb/avb_util.cpp).

Read-only listing of the stock vendor_boot and init_boot ramdisk archives
found no entry containing `avb`. This is limited to those archives, not proof
that no keys exist elsewhere in the firmware. It grants no new trust path.

The same 2026-10-25 archive has divergent boot-state observations:
`oplusboot.verifiedbootstate=orange` in kernel-runtime.txt, versus
locked/green/enforcing userspace properties in getprop.txt. The vendor
`oplusboot` namespace is not the AOSP `androidboot`/`ro.boot` lookup: that
orange token alone does not establish AOSP permissive mode, and OEM translation
is unverified. File modification times are archival metadata, not authenticated
state attestations. No serial,
PIN, full command line, encryption table or raw device-property dump is
included here. No ADB or fastboot device was visible during this audit.

## 4. Signing authority: supported mechanisms versus unverified assumptions

1. **Existing stock authority:** an updated root descriptor needs a matching
   top-level signature accepted by this device's trust policy. K0's public key
   and valid existing signatures do not provide its private signing authority.
   No OEM signing service/private key is established by this project.
2. **User-enrolled root:** Android documents this as optional, with Pixel as
   an implementation example. No official OnePlus15/ColorOS17 documentation,
   exact bootloader source, or observed enrollment capability establishes it
   here. Do not infer it from another OnePlus model or Pixel.
   [AOSP device-state documentation](https://source.android.com/docs/security/features/verifiedboot/device-state#user-settable-root-of-trust)
3. **Chained signing:** a child is trusted because an accepted parent pins
   its key/location. A new child or re-signed vbmeta_vendor cannot substitute
   for the direct root descriptor in the current stock parent. An intentional
   new delegation would itself need accepted parent authority.
   [AVB architecture](https://android.googlesource.com/platform/external/avb/+/master/README.md#The-VBMeta-struct)
4. **Unlocked development:** AOSP permits certain verification errors in its
   unlocked model. That is not authenticated enrollment, and it does not
   establish this OEM's boot policy or first-stage mount acceptance. Successful
   flashing and a valid self-signature are insufficient evidence.
   [AVB integration contract](https://android.googlesource.com/platform/external/avb/+/master/README.md#Locked-and-Unlocked-mode)
5. **Full custom signing:** rebuilding more partitions does not create trust.
   It may be an option only after a supported root and complete reviewed
   boot/FBE/rollback contract exist. It is not proven necessary or sufficient
   here; a larger package must not be proposed as an automatic workaround.

The official pinned OnePlus manifest provides kernel/SoC/module/build-tool
sources, not the shipping ABL implementation or a key-enrollment procedure.
See the [manifest](https://github.com/OnePlusOSS/kernel_manifest/blob/ecdd4809cbd01fa5f509a4708347026f63498303/oneplus_15.xml)
and [README](https://github.com/OnePlusOSS/kernel_manifest/blob/ecdd4809cbd01fa5f509a4708347026f63498303/README.md).
Bounded inspection of the downloaded stripped ELF32 ABL found no plain
ASCII/UTF-16 custom-key handler evidence. String absence does not prove that
the implementation lacks the feature. The public OnePlus T-Mobile unlock
page is not a CPH2745 custom-root support statement.

Existing PKCS#7 kernel module-signing authority is a different trust domain
from AVB. It cannot sign the stock top-level parent under K0. The already
qualified custom boot's AVB key is likewise different, as tested below.
Do not replace the bootloader, change fstab/verification code, relock, enroll
a key or change rollback state merely to probe this unknown support.

## 5. Offline validation performed

The manifest avbtool was run against original images through a private flat
image view. Each expected parent chain key/location was supplied explicitly
for the parent pin check; each raw parent-pinned key was independently compared
with the corresponding stock child's embedded key.

| Offline check | Result and meaning |
|---|---|
| info_image for stock VBMeta/footer objects | PASS; exact topology/geometry parsed |
| Stock top-level signature with explicitly extracted K0 public key | PASS; signature and selected key match |
| All five parent chain key/location records and child key bytes | PASS; relative stock delegation is consistent |
| Stock chained child signatures and their referenced hash/hashtree images | PASS; internally consistent stock image set |
| Direct init_boot/vendor_boot hashes and both DLKM trees via top-level parent | PASS |
| Each DLKM footer and hashtree checked individually | PASS for integrity; Algorithm NONE is not independent authentication |
| Top-level verification without expected chain records or follow flag | Expected FAIL; missing expected boot chain record |
| Qualified custom boot with its own extracted public key | PASS for self-signature/integrity |
| Stock top-level vbmeta checked against qualified custom-boot public key | Expected FAIL: embedded key mismatch |
| Qualified custom boot checked against K0 | Expected FAIL: embedded key mismatch |
| Sixteen source-image hashes before/after | IDENTICAL; no source image was written |
| Hardware trust anchor, enrollment and stored rollback values | NOT VERIFIED offline |
| FEC parity recomputation | NOT PERFORMED; geometry only is recorded |
| Device custom-DLKM acceptance and runtime tests | NOT TESTED |

The qualified boot's public-key SHA1 identifier is
`2597c218aae470a130f61162feaae70afd97f011`, not K0. This contrast does not
invalidate the previously working boot; it demonstrates why its successful
deployment is not proof of top-level OEM signing authority or a strict
custom-root boot chain.

`verify_image` normally checks a signature against an embedded key; `--key`
adds key comparison. `--follow_chain_partitions` alone is not a substitute
for explicitly validating the expected parent pins. No command here proves
that the device has authorized a newly generated key. See
[official avbtool verification semantics](https://android.googlesource.com/platform/external/avb/+/master/README.md#Using-avbtool).

**No development signing example was created.** The phase3 prerequisite—an
established supported signing method—was not met. Producing a self-consistent
test-key package would not resolve that missing authority and is deliberately
not presented as a device candidate. No private key or new image was generated.

## 6. Deployment options and exact affected partitions

| Option | Required authority/prerequisites | Affected partitions | Current decision |
|---|---|---|---|
| New vendor_dlkm only, original vbmeta | New data would have to satisfy the unchanged authenticated stock root | vendor_dlkm in selected slot | BLOCKED for a real changed filesystem; a signed footer alone does not satisfy the old parent |
| New vendor_dlkm + matching top-level vbmeta | OEM-authorized K0 signature, or a separately proven supported enrolled root; retain all unrelated descriptors/keys/rollback policy | vendor_dlkm and vbmeta in the same selected slot; key provisioning if actually supported is a separate security-state change | Minimum theoretical matched image set; authority NOT ESTABLISHED |
| New vendor_dlkm + only vbmeta_vendor | Stock direct top-level vendor_dlkm descriptor would remain stale | Those two files alone do not update the real parent | NOT A SOLUTION under current topology |
| New dedicated trusted child vbmeta | Accepted root must first authorize a new delegation; existing partition/layout/first-stage support must be proven | vendor_dlkm, top-level vbmeta, plus an actual supported child location | NOT ESTABLISHED; no new partition or delegation is created |
| Larger matched ColorOS package/custom signing | Complete supported trust, firmware, FBE, rollback and dependency contract | Cannot justify a fixed expanded set without that design | BLOCKED; more images do not solve missing signing authority |
| Leave qualified boot + original stock DLKM | Existing qualified contract | No new writes | Preserve current production baseline |

The reviewed RMNET source change itself needs only the actual vendor_dlkm
`rmnet_core.ko` replaced; its kernel and retained providers remain compatible.
There is no proven need to change system_dlkm, vendor_boot, init_boot, vendor,
DTBO, radio, persist or userdata. No such change is authorized by this report.

Any future accepted package must preserve ext4 format, labels/ownership,
module paths/load order/dependency metadata, natural CRCs/signing class and
verity/FEC layout or a reviewed equivalent. Validate actual logical capacity,
slot mappings and super constraints before packaging; image-file size alone
is not live partition capacity. No resizing is proposed.

## 7. Remaining blockers, rollback and next action

1. Establish exact-device signing/enrollment support from manufacturer evidence
   or a reviewed implementation, rather than assuming unlocked acceptance.
2. When a device is available, obtain only scoped read-only bootloader and
   first-stage AVB diagnostics. No reboot, enrollment, relock or write is
   performed as an implicit part of this offline task. Mutable Android
   properties alone are insufficient; record unknown/unsupported query
   responses without interpreting them as proof of enrollment support.
3. If a supported authority is established, design the minimum matching
   vendor_dlkm + parent package, then perform the separately scoped offline
   development signing example with that topology. Preserve all stock
   unrelated descriptors, firmware properties, algorithms and rollback rules.
4. Only after static image/AVB/module gates pass should an exact partition set
   be proposed for separate physical authorization. Root-of-trust changes may
   affect attestation/KeyMint/FBE; they are not a harmless packaging detail.

Before any later authorized canary, preserve full current-slot vbmeta and
vendor_dlkm read-backs, their exact hashes and the original qualified boot.
Keep inactive-slot rollback intact and do not reboot between an inconsistent
partial image set. Restore both changed parent and filesystem together if
necessary, then verify read-back, Android, user0 and F2FS/inlinecrypt. Never
wipe metadata/userdata or reset indexes as recovery shortcuts. The qualified
boot SHA256 remains
`1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3`.

This phase does not rerun or alter the existing RMNET 1070-module oracle,
private-layout/KCFI evidence or40 host/static tests. The common kernel remains
6.12.114; the38 reviewed enforced GKI differences remain FAIL, not hidden by
this AVB audit. See the [staged driver report](coloros17-sm8850-final-report.md)
and [existing rollback plan](coloros17-test-and-rollback-plan.md).

**Final status: BLOCKED — authenticated vendor_dlkm deployment authority and
exact-device support remain unestablished.**

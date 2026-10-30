# ColorOS 17.0.0.102 four-provider offline filesystem experiment

Date: 2026-10-30. This is **not authorized for flashing**. Source compilation,
stock-module compatibility and filesystem-fit checks pass for the four named
providers. Actual LP/snapshot validation, parent AVB authorization and physical
qualification remain blocked/not tested. No production image or partition was
modified, no device was written, and no package was published.

## Exact scope and providers

The frozen qualified common kernel remains
`818ee1b0ff65d6d475afeeba55bf4099cfcbfc22`, release
`6.12.114-android16-6-o-gb6bd4a40410d-4k`. This experiment does not rebuild or
replace its boot image. The only intended filesystem delta is four files in
the stock ColorOS `CPH2745_17.0.0.102(EX01)` vendor-DLKM image:

| Actual partition/file | Stripped bytes | SHA256 |
|---|---:|---|
| `vendor_dlkm:/lib/modules/rmnet_core.ko` | 472,168 | `0ad99c698fcaffdf0acd2656b8e4c4a113ec41c173828f4dec40ea25e6d5e287` |
| `vendor_dlkm:/lib/modules/qce50_dlkm.ko` | 146,192 | `eea2b5252aa6838b1091af37fa901827254cf33e7707cd5113d7eaa0aead5f62` |
| `vendor_dlkm:/lib/modules/qcrypto-msm_dlkm.ko` | 150,240 | `6606b6a4bcb9818271015f66658c201020a1a9761bbb37554020ad8e65fa0bfb` |
| `vendor_dlkm:/lib/modules/wcd9xxx_dlkm.ko` | 50,208 | `5e10de199ea0eb39cde0d4c771995851f9dfc6c284eb8a0b98fc595cbfe5bec4` |

All four have stock UID/GID `0:0`, mode `0644`, and exact SELinux xattr
`u:object_r:vendor_file:s0`. They are uncompressed ELF modules. No module is
re-signed, no signature/CRC/vermagic/load-policy check is bypassed, and the
new files retain the established unsigned-vendor signing class.

The actual stock `vendor.img` EROFS path `/lib/modules` is a symlink to
`/vendor_dlkm/lib/modules`; thus the `/vendor/lib/modules/...` names in
`modules.dep` do not select another old stock copy. This was read directly
from that image's EROFS inode/directory data using the checked-in kernel's
on-disk definitions; vendor image SHA256:
`c084799e0b245c546c663f88a0a6110b8f497086387a5262ddad1c7caffbceae`.

The complete 1,070-location inventory contains exactly one copy of each of
these four canonical providers, all in vendor-DLKM. None occurs in
`system_dlkm` or `vendor_boot`; neither vendor-boot normal nor recovery load
list requests them. Normal Android loading therefore selects the replacement
at the existing path. No replacement recovery provider is claimed, and a
fresh device/recovery load trace has not been performed.

There are 89 uniquely supplied export-import edges: RMNET 56, QCE 22, WCD
11. The exact consumer/symbol/CRC/provider list is
[coloros17-four-provider-edges.tsv](coloros17-four-provider-edges.tsv).
QCrypto has no external module export-import edges in this population; its
algorithms are registered through the existing kernel crypto API. No oracle
fallback to an unchanged old copy of any of these four providers is possible
within this package.

FastRPC, GPU, camera, video and all SDK dependency modules are **not**
substituted. Their source/control investigations do not become deployed
security claims merely because they compile.

## Construction and complete readback

Original downloaded vendor image:
`/home/travis/Downloads/CPH2745IN_17.0.0.102/vendor/vendor_dlkm.img`.
SHA256: `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47`.

Private output/evidence:
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/security/offline-vendor-dlkm-run06/`.

The read-only source inventory records all 476 inodes' stock UID/GID, modes
and SELinux xattrs, including root and `lost+found`; all 471 file paths are
extracted and hashed, including `/etc` and every module metadata file. Only
the four module files above are replaced. Qualified Clang-r536225
`llvm-strip --strip-debug` removes debug information from private module
copies, preserving every allocated runtime section and true export, import,
version and namespace record. The LLVM-strip executable SHA256 is
`6dd7820310e2349581a5a450363daff5aebfa05aef740c0f700ad44310d06477`.

A **fresh** filesystem is built with the existing official
`mkuserimg_mke2fs`/`mke2fs`/`e2fsdroid` tools, normal deduplication
`--share_dup_blocks`, and per-path `fs_config`/SELinux contexts generated
from the original image. It is not an in-place edit of ext4 shared blocks.
The build runs under `fakeroot`: AOSP `perms.c` deliberately does not chown
`lost+found`, so relying on unprivileged host extraction ownership would
otherwise leak UID 1000 there. The fresh final image passes all 476 exact
UID/GID/mode/xattr comparisons.

The filesystem retains stock label, mount point, UUID, directory-hash seed,
features (including shared blocks), block/inode geometry, mount options,
reserved-block count and file count. Its fixed build timestamp uses the
stock file epoch `1792257367`; filesystem creation/check/write timestamps
and lifetime-write accounting naturally differ from the original image.
Those differences are documented, not claimed byte-identical metadata.

| Filesystem/envelope check | Result |
|---|---|
| Exact ext4 data size | 152,317,952 bytes = 37,187 × 4,096; unchanged |
| Exact full image size | 154,849,280 bytes; unchanged |
| Allocated blocks | 37,076; stock 37,075 |
| Free blocks | 111; stock 112 |
| Inodes / used / free | 512 / 485 / 27; unchanged |
| Read-only forced `e2fsck -f -n` | PASS, no repair applied |
| Final post-AVB filesystem extraction | All 471 file hashes checked; only exact four module changes |
| All module load/dependency/alias metadata | Byte-identical to stock |
| All inode UID/GID/mode/SELinux identities | 476 / 476 match stock |

Failed private setup/build attempts are retained separately: initial
debugfs-format/empty-entry parser rejection, canned-root-path configuration,
the caught `lost+found` UID mismatch, and missing host `fec` PATH input.
None was admitted as a qualification pass or fixed by changing source
filesystem ownership defaults or enlarging a partition. The successful
command and manifests are in the private run06 evidence directory.

The successful filesystem command, after the exact stock metadata/context
files and staged four-module tree have been generated and verified, is
shown below. `NEW` must be a **new private directory**, never the stock
download or an existing qualified artifact. This is not a flashing command.

```sh
set -eu
TOOLS=/path/to/reviewed/kernel_platform/prebuilts/kernel-build-tools/linux-x86/bin
NEW=/path/to/new/private/filesystem-experiment
test ! -e "$NEW/vendor_dlkm.img"
PATH="$TOOLS:$PATH" fakeroot -- "$TOOLS/mkuserimg_mke2fs" \
  --journal_size 0 --timestamp 1792257367 \
  --fs_config "$NEW/fs_config.txt" --label vendor_dlkm \
  --inodes 512 --inode_size 256 --reserved_percent 0 \
  --mke2fs_uuid 62e5740c-e8cd-5354-96b9-e325d6634a4e \
  --mke2fs_hash_seed abb0378d-364d-5310-9641-9a6921436853 \
  --share_dup_blocks "$NEW/tree" "$NEW/vendor_dlkm.img" \
  ext4 vendor_dlkm 152317952 "$NEW/file_contexts.txt"
"$TOOLS/e2fsck" -f -n "$NEW/vendor_dlkm.img"
```

The canned-config entry for the filesystem root is `/`, not `vendor_dlkm`:
AOSP `perms.c` maps only that inode's lookup name to the empty string, and
`canned_fs_config.cpp` represents it as `/`. Other entries use exact
`vendor_dlkm/...` paths; SELinux expressions use exact `/vendor_dlkm/...`
paths. No broad default-owner or generic module-label rule is substituted.
The command alone is insufficient: complete final file/xattr readback,
normal footer construction, independent integrity checks and the final
1,070-location oracle remain required.

## AVB integrity is not parent authorization

The normal AVB tool regenerates the offline image's SHA-256 hashtree and
two-root FEC, keeping all stock outer geometry/properties:

| AVB field | Preserved value |
|---|---|
| Algorithm / rollback index / location / flags | `NONE` / `0` / `0` / `0` |
| Release string | `avbtool 1.3.0` |
| Data/tree block size | 4,096 bytes |
| Tree offset / size | 152,317,952 / 1,208,320 bytes |
| FEC offset / size / roots | 153,526,272 / 1,220,608 bytes / 2 |
| VBMeta offset / size | 154,746,880 / 768 bytes |
| Salt | `87945e0bf09af70da8e6c38180950738c060aba9b1ef8f25f071e7e39375fee3` |
| OS version property | `16` |
| Fingerprint property | `qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys` |

`avbtool verify_image` verifies the offline NONE vbmeta/footer and SHA-256
hashtree. Independently re-encoding the actual 153,526,272-byte data-plus-tree
prefix with the qualified `fec --encode --roots 2` produces exactly the
stored 1,220,608 parity bytes; the image hash is unchanged by verification.
No OEM key, fake signature, rollback change, or AVB-disabling flag is used.

The root digest necessarily changes from stock
`1374ff0726349e3ca91b2999a786c12af12530aff39a836c58dae68839f1669c`
to `6d38272aa876b7a3eb84e35d0f7335d38f044e43c19c9bb419e30ca4ff638000`.
The unchanged parent vbmeta still authenticates the **stock** descriptor.
Passing an image's own unsigned integrity checks does not authorize this new
root under the device trust chain. That deployment gate remains **BLOCKED**.

Offline candidate image SHA256:
`b8a5385cb563188640395b4117681b782f8e3a97fcd914d8ce6a34af0e6e62db`.

## Final extracted-filesystem stock-module gate

Evidence:
`security/integrated-oracle-final-filesystem-run02/` under the private
modernization archive. Inputs are the **post-strip, post-AVB extracted
vendor filesystem**, plus the exact unchanged stock system-DLKM and
vendor-boot population and the frozen qualified vmlinux. The hybrid is both
consumer and provider universe. No cached SDK module substitutes for a
shipping provider.

| Gate | Observed result |
|---|---|
| Locations / active compatible / dormant | 1,070 / 1,032 / 38 |
| Hard imports | 60,701 / 60,701 |
| All undefined / weak | 60,707 / 6 |
| CRC records | 61,777 / 61,777 |
| Missing / CRC / namespace / active blockers | 0 / 0 / 0 / 0 |
| Actual vmlinux-first edges | 54,785, zero production-provider mismatches |
| Compiled unsigned-import-policy edges | 54,986, zero failures/protected collisions |
| Unchanged ambiguous-provider edges | 3,220, zero new/changed ambiguity |
| Unique replacement export-import edges | 89; zero competing old provider copies |
| SDK provider substitutions | 0 |

This is an offline filesystem-size-envelope and complete static-module
qualification result. Actual device super/LP metadata and fresh snapshot
state are unavailable; stock image size is **not** a substitute for that
capacity gate. No viable device flash procedure is authorized by this
experiment. Boot, system-DLKM, vendor-boot, init-boot, DTBO, VBMeta and device
partitions remain untouched. Physical driver functionality, fault injection,
radio/tethering, audio and suspend tests are **NOT TESTED**.

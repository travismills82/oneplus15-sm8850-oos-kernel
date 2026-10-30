# ColorOS 17.0.0.102 driver modernization: preserved baseline

Audit date: 2026-10-30. This project targets ColorOS only. No LineageOS build,
configuration or deployment dependency is introduced. Lineage history, when
used, is only a locator for authoritative Qualcomm provenance.

## Immutable starting point

| Item | Verified value |
|---|---|
| Experimental branch | `experiment/coloros17-sm8850-dlkm-updates` |
| Starting release/source HEAD | `efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c` |
| Content-equivalent kernel source commit | `10b349653c7b3a3a1e55d2fd357ed7220190c1b9` |
| Common source tree | `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22` |
| Linux / KMI | 6.12.114 / Android 16 generation6 |
| Framework | Android17 / SDK37 |
| Vendor layer | Android16 / SDK36 |
| Firmware | `CPH2745_17.0.0.102(EX01)` |
| Qualified kernel release | `6.12.114-android16-6-o-gb6bd4a40410d-4k` |
| Exact boot SHA256 | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| Embedded Image SHA256 | `8ebe51e2a565a55886245793f572661e87c1a40bf5016bb79b84855370242456` |
| Module.symvers SHA256 | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Common effective config SHA256 | `e5495ab30241f869f12b3fc2ff3590886150fb464c736975c8054303a9e9981a` |
| Runtime build ID, preceding physical test | `631855c02d3a5c50f57eedc2d640554570f807ee` |

Framework/vendor Android versions are from the exact firmware's archived
properties, not inferred from its marketing name. The mixed17/16 contract is
intentional evidence; neither a new KMI generation nor an arbitrary newer
vendor release follows from ColorOS17 alone.

The source checkout was clean before audit work. The unrelated heavily dirty
workspace root and the released worktree remain untouched. The new worktree
is `out/coloros17-sm8850-dlkm-updates`; its initial common tree matches the
released source. No Linux advancement or source/config change is part of
baseline preservation.

## Rollback copies and evidence

Owner clarification during this audit: OnePlus does not currently supply a
source archive or manifest for this exact firmware. No exact vendor Git
revision is fabricated. Per-module reconstruction, compiled-code/boundary
comparison and private-ABI evidence are therefore essential before selecting
replacements; matching a release/version string alone is insufficient.

Fresh independent copies were made under:

`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-baseline-20261030/`.

The six-entry `baseline-sha256.txt` verifies the following four images plus
the preserved config and Module.symvers. The stock image inputs are the full
firmware images in `/home/travis/Downloads/CPH2745IN_17.0.0.102/`.

| Backup | Bytes | SHA256 |
|---|---:|---|
| `images/qualified-boot.img` | 100663296 | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| `images/stock-system_dlkm.img` | 14512128 | `3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a` |
| `images/stock-vendor_dlkm.img` | 154849280 | `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47` |
| `images/stock-vendor_boot.img` | 100663296 | `d0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000` |

The full stock hashes equal those verified on the phone in the preceding
physical canary. Older files called `*-verity.img` are decrypted filesystem
views, with different lengths/hashes; they must not substitute for complete
rollback partition images.

The preserved original evidence roots are not modified:

- `kernel-test-backups/oos1700102-baseline-20261025-120000/`: exact stock
  modules, early-boot ramdisk, original module metadata and firmware properties.
- `kernel-test-backups/coloros1700102-6.12.114-five-cves-20261029/`: frozen
  build outputs, toolchain hashes, configuration, ABI and oracle evidence.
- `kernel-test-backups/coloros1700102-6.12.114-five-cves-physical-20261030-055620/`:
  full boot read-back, protected partition hashes and physical observations.

No ADB device was visible at audit start. This is preservation of known exact
firmware images and the recorded prior physical state, not a fresh read-back
from an unseen phone. No flash/reboot or physical driver test is claimed.

## Production build and unchanged gates

The source configuration pipeline is common `gki_defconfig` plus the Canoe
perf/config/module definitions under `kernel_platform/soc-repo`, OPlus build
integration, and external-driver configuration. A module's local build defines
must also match; copying common .config alone is insufficient.

Production target: `//soc-repo:canoe_perf_dist`. Device-inclusive ABI dump:
`//soc-repo:canoe_perf_abi`, which is not itself an enforced device diff.
The enforced comparator `//common:kernel_aarch64_abi_diff` has the existing
**FAIL/exit4 with38 reviewed differences**. The reference remains frozen;
zero new differences, not a fabricated overall PASS, is the starting gate.

The existing stock oracle accounts for1070 partition-qualified modules:
103 system_dlkm,461 vendor_dlkm,506 vendor_boot;702 unique canonical names.
It reports1032 active compatible,38 dormant,60701/60701 hard imports and
61777/61777 CRC records, with zero missing/CRC/namespace/protected/active or
production-provider blockers. Duplicate names require real loader precedence,
not a convenient compatible provider selected by an oracle.

Signing policy remains103 trusted signed stock modules plus967 retained
unsigned modules. MODULE_SIG, MODVERSIONS, GENDWARFKSYMS, trimming and CFI remain
enabled. The qualified common configuration uses LTO_NONE and SHA1 module
signing; this audit neither changes those existing values nor claims SHA512
production signing. Public-key/AVB algorithm identity is a different gate.

## Validated boot AVB contract

The preserved boot uses header4, empty ramdisk/command line, 96MiB capacity,
SHA256_RSA4096, public-key SHA1
`2597c218aae470a130f61162feaae70afd97f011`, flags0, rollback index0/location0.
Outer properties remain OS16, security patch2026-10-01, fingerprint
`qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys`.
Do not change these to match a marketing version or invent a newer patch date.
DLKM parent-vbmeta validation is separate and mandatory before packaging.

## Conntrack is separate and already repaired

The request's statement that CVE-2026-80668 remains unresolved describes an
older artifact, not this baseline. Commit `633ac4e5f56b64aa9cf80d5813c6fac82cf43d82`
contains the reviewed local lifetime/GC compatibility port based on upstream
`b8b09dc2bf35a00d4e0556b5d6308c7b917ebda2` and stable6.12 reference
`02f7809a19fc88cfd9bc113a4bfe7add6305db39`. It is an ancestor of the baseline.
Production compilation, unchanged external ABI, helper/NAT/lifetime KASAN
tests and recorded physical FBE/network canaries support that repair.

No vendor-driver update gets credit for the conntrack fix. Keep it and all
five later CVE backports intact. This is not an exhaustive security-clearance
claim for stock modules, firmware or the kernel.

The newest boot passed its bounded610-second Android/FBE/module canary, not
every peripheral/suspend gate. Earlier owner hardware results remain attached
to their exact earlier artifacts. A new matched driver package needs its own
complete physical qualification after explicit authorization.

The owner describes the current baseline as hardware-tested and stable in the
new task. That owner statement is retained separately from the detailed
automated measurements above; it does not qualify any newly rebuilt driver.

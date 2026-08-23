# Controlled-stack TWRP ZIP release contract

## Policy

Every controlled-stack release must be delivered and validated as one TWRP
ZIP. A directory of loose partition images is not a releasable artifact.

The ZIP must contain the exact physically qualified payloads and use the
hardened controlled-stack helper. The helper must fail closed on device,
slot, snapshot, capacity, filesystem, backup, write, or read-back validation
failure.

Required write scope and order:

1. `vendor_dlkm`
2. `system_dlkm`
3. `boot` last

`vendor_boot`, VBMeta, `system_dlkm_oki`, and slot metadata are outside this
release contract and must not be included or modified.

## Mandatory gates before release

1. Build the archive from the exact physical-baseline manifest and report.
2. Verify every embedded payload byte-for-byte against its qualified input.
3. Verify ZIP integrity, internal `SHA256SUMS`, executable modes, write scope,
   write order, and the pinned hardened-helper hash and lineage.
4. Verify boot AVB and both DLKM AVB hashtrees/footers.
5. Rebuild the ZIP independently and require a byte-identical archive.
6. In live TWRP, verify the device, slot, snapshot state, decrypted durable
   backup destination, target capacities, and filesystems; then run the exact
   ZIP entrypoint in `--dry-run` mode and prove no partition changed.
7. Before publishing a release, install that exact ZIP once through its TWRP
   entrypoint with full verified backups and exact partition read-back hashes,
   then boot and re-run the release regression gates.

If any gate is missing, the package may be retained as a development artifact
but must not be called or published as a release.

## Current development package

Package identity:
`controlled-v1-g6744-wlan053-bt046-nfc102-cellular102-audio059-graphics057-camera073-platform086`

Exact inputs:

| Payload | SHA-256 |
|---|---|
| `boot.img` | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| `system_dlkm.img` | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| `vendor_dlkm.img` | `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26` |

Generated ZIP SHA-256:
`a603e6af54325c86e20182ad0d24e91c7d5743697a1c51a6ec4bdcf7424bd8ed`

Static result on 2026-08-23:

- exact physical payload match: PASS
- deterministic rebuild: PASS
- ZIP integrity and internal checksums: PASS
- boot AVB verification: PASS
- system_dlkm hashtree verification: PASS
- vendor_dlkm hashtree and ext4 verification: PASS
- hardened-helper pin and negative rejection tests: PASS
- live TWRP ZIP dry run: PASS — full ZIP entrypoint; no partitions modified
- live TWRP ZIP installation: PASS — normal `twrp install` path
- full verified backups: PASS
- post-write partition read-back: PASS
- post-install Android boot and core runtime contract: PASS

This package satisfies the TWRP flashability gates. It remains a development
artifact because no release was requested or published.

## Live TWRP proof

The exact ZIP above was tested on 2026-08-23 with TWRP
`3.7.1_16-OnePlus_15` on CPH2747 slot `_b`.

- user 0 decryption and writable durable backup guard: PASS
- device, slot, and no-snapshot guards: PASS
- one-ZIP dry run: PASS
- one-ZIP install through `twrp install`: PASS
- backup directory:
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260823-180400`
- backup source/read-back equality: PASS for all three partitions
- installer write order: `vendor_dlkm`, `system_dlkm`, `boot` last
- active-slot metadata change: none
- vendor_boot/VBMeta/system_dlkm_oki writes: none
- post-install boot: PASS
- post-install kernel: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- post-install payload read-back: exact physical hashes
- system load contract: 46 entries; `wwan.ko` entry 21
- WLAN: 6135 MHz WPA3 and IP/DNS PASS
- cellular: Visible LTE HOME/IN_SERVICE; dual-stack RMNET PASS
- module contract error scan: PASS

## Build and validation entrypoints

Use:

```text
tools/package-controlled-oos-stack.sh --profile controlled-v1-modernized-twrp ...
tools/validate-controlled-stack-twrp-zip.py <zip> --boot ... --system-dlkm ... --vendor-dlkm ... --helper ...
```

The build refuses a payload that does not match the reviewed physical
baseline, a non-hardened helper, an unsafe release tag, or an output directory
that already exists.

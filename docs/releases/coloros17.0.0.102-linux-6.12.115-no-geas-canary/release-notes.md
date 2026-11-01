# OnePlus 15 ColorOS 17.0.0.102 — Linux 6.12.115 updated-Adreno canary

**Experimental prerelease. GEAS is intentionally disabled. Not a stable release.**

**Restricted existing-matched-installation package: NOT installable over stock
vbmeta.** At the owner's request, vbmeta is neither included nor written. The
ZIP requires the matching parent already installed, the tested vendor image
already present, and the exact companion images/layout below. It rejects stock
or unmatched metadata. This asset set is a reinstall/update package for the
existing matched canary, not a way to install its driver stack on an untouched
ColorOS installation. Do not bypass the checks or disable verified boot.

## Assets and identity

- `boot.img`
- `vendor_dlkm.img`
- `Kernel-ColorOS-17-17.0.0.102-6.12.115-no-GEAS-canary-twrp.zip`
- `canary-manifest.json` and `SHA256SUMS`

Kernel: `6.12.115-android16-6-o-ga3b501fee3c3-4k`.
Exact kernel build parent: `a3b501fee3c3c9e44579f46fd78e2282d05e29e8`.
The later release-engineering commit/tag records the packaging and reports;
the physically tested kernel/module bytes were not rebuilt or renamed internally.

| Payload | SHA256 |
|---|---|
| boot.img | `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9` |
| vendor_dlkm.img | `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804` |

This is **Linux 6.12.115**, not .114 or a new untested kernel rebuild.
The kernel includes the preserved upstream interval, ColorOS KABI repairs and
the prior conntrack/security fixes. Updated Adreno is included with the four
earlier RMNET, QCE/QCrypto and WCD9xxx canary replacements. Other quarantined
driver-source changes are not silently deployed. GEAS CPU/system/data modules
are deliberately excluded/blacklisted; their original getter ABI is not fixed.

## Required existing environment

- OnePlus 15 CPH2745IN / Canoe / infiniti, ColorOS
  `CPH2745_17.0.0.102(EX01)`, tested **active slot B**.
- Already-installed parent vbmeta B SHA256:
  `54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c`.
- Existing vendor_dlkm matches the candidate hash above, so immediate rollback
  remains consistent with that unchanged parent.
- Tested TWRP `3.7.1_16-OnePlus_15`, recovery kernel
  `6.12.114-android16-6-o-gb6bd4a40410d-4k`.
- Bootloader already unlocked, raw ORANGE boot state. Magisk-spoofed Android
  GREEN/locked properties are not used as bootloader evidence.
- Exact tested stock system_dlkm, vendor_boot and DTBO; unchanged tested
  Magisk-patched init_boot B. Their expected hashes are in `canary-manifest.json`.
  A different Magisk build/patch or stock init_boot is not qualified by this ZIP.
  Do not overwrite init_boot merely to satisfy the guard.
- Exact tested super metadata and linear vendor allocation: 154,849,280 bytes,
  302,440 sectors at super sector 22,396,928. No resize, extra vendor alias,
  holder, mounted vendor filesystem, active OTA/snapshot or unknown snapshot state.
- Decrypted user0 and mounted F2FS /data for verified durable backups. At least
  768 MiB available RAM, 300,000 KiB tmpfs space and sufficient backup storage.
- At least 30% battery charge and battery temperature no higher than 45 C.

These deliberately narrow guards do not establish support for every OnePlus 15,
regional model, firmware, recovery or partition layout. **If a guard rejects the
device, stop. Do not switch slots, change keys, alter verification, wipe data or
manually flash these loose images to bypass it.**

## TWRP installation

The new ZIP has host fixture/static and deterministic-archive validation only;
**the ZIP entrypoint itself has NOT been physically installed**. The embedded
image pair has physical-canary evidence with its pre-existing matching parent.
This distinction is why the release remains an experimental canary.

1. Preserve host-side known-working recovery and a coordinated boot/vendor/AVB
   rollback for your existing installation. Back up important user files too.
2. On the PC, download the assets into one folder and verify:

   ```sh
   sha256sum -c SHA256SUMS
   ```

3. Enter the specified existing TWRP on the already active B slot. This ZIP
   does not install recovery. Decrypt with **your own** device PIN and confirm
   internal storage is readable. Never format /data or /metadata.
4. Ensure vendor_dlkm is unmounted and has no mapping holders. Use TWRP Install
   to select `Kernel-ColorOS-17-17.0.0.102-6.12.115-no-GEAS-canary-twrp.zip`.
5. The installer checks images, firmware, existing authentication metadata,
   companions, slot, super layout, snapshots and durable storage. It creates
   complete verified boot/vendor backups under:
   `/data/media/0/TWRP/kernel-flash-backups/coloros17-linux115-no-geas-b-.../`.
6. It writes **vendor_dlkm B first, boot B second**, with full SHA256 readbacks
   after each write and protected-partition checks. It never writes vbmeta,
   init_boot, system_dlkm, vendor_boot, DTBO, recovery, slot/super metadata or
   encryption metadata, and never reboots automatically.
7. Only after `INSTALL PASS`, select **Reboot System** without switching slots.
   Verify the kernel identity, encrypted user0, Wi-Fi/BT/mobile data and GPU.
   Keep the backup directory and logs. Report any failure against these exact
   artifact hashes.

For an optional non-writing recovery preflight, extract the ZIP's update-binary
into /tmp and run it with `COLOROS115_CANARY_MODE=dry-run`, arguments `3 1` and
the full ZIP path. It performs the same read-only prerequisite checks without
backups or partition writes. This does not substitute for physical ZIP validation.

Loose images are supplied for integrity inspection and expert recovery, not as
generic fastboot commands for stock-parent devices. In particular, fastboot
write success does not establish dm-verity mounting or encrypted Android boot.

## Rollback

For a write/readback/interruption failure, the installer attempts to restore
both immediate pre-install backups (vendor first, boot second) and verifies
them, leaving the parent unchanged. If it reports rollback failure, **do not
reboot**: retain the backup directory, recovery and host connection.

For a later Android failure, use the previously prepared host/recovery plan to
restore those verified immediate boot/vendor backups in the same order; keep
their existing matching parent and init_boot unchanged. Recheck slot, snapshots,
capacities and every full readback. A/B vendor mappings share backing storage;
switching slots is not rollback. Never restore stock vendor_dlkm alone under
the current experimental parent. Returning to the independent original stock
or .114 three-image arrangement requires its coordinated metadata restoration,
which is not supplied or performed by this two-image ZIP.

## Qualification and known limitations

- Exact installed boot/vendor/parent readbacks, Android startup, user0/FBE,
  F2FS inlinecrypt and live vendor dm-verity V/0: PASS in the recorded canary.
- Actual GLES and Vulkan graphics rendering: eight bounded runs, 480.073 seconds,
  470,745 frames, 32 context/device cycles, zero pixel/test-memory failures.
- Owner-confirmed Wi-Fi/BT reconnect, mobile data/call, camera/video,
  speaker/microphone, fingerprint and OTG read/eject: PASS on the test device.
- GEAS-excluded enabled population: 1,064 records, 1,029 active compatible,
  35 dormant; 60,267/60,267 hard imports and 61,337/61,337 CRC records match;
  zero missing providers, CRC/namespace failures or active blockers.
- Shared GPU KCFI prefixes and reviewed layout/UAPI checks passed. Stock GPU
  lacks complete private DWARF/BTF; no exhaustive private-layout claim is made.
- Enforced GKI comparison remains **FAIL**: 38 prior reviewed differences plus
  six documented .115 representation differences. Strict KMI/ABI generation
  PASS is not overall GKI PASS. The ABI reference was not changed.
- Six startup WARNs and one additional stock schedinfo WARN are documented.
  Camera/HFI/UART/touch diagnostic limitations remain visible; no suppression
  or clean-WARN claim. No captured GPU fault/panic/Oops/KCFI/module failure.
- USB-disconnected deep suspend, measured GEAS-enabled performance/power
  equivalence, cooling-request oscillation impact and exhaustive peripherals
  remain open. Successful benchmarks are not a frame-pacing/battery comparison.
- Conntrack CVE-2026-80668 repair is preserved; this is not an exhaustive
  security-completeness claim for ColorOS proprietary firmware or retained modules.

The stable .114 release stays latest stable. This canary does not qualify
permanent GEAS removal, universal compatibility or OTA support. Do not install
a ColorOS OTA, relock the bootloader or enroll AVB keys as part of this test.

Source details and full validation are in the repository's
`docs/validation/firmware-17.0.0.102/coloros1700102-linux-612115-no-geas-extended-qualification.md`.

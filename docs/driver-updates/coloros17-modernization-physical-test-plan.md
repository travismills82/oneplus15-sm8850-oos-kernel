# ColorOS 17 modernization: future staged physical qualification

**Not authorized for deployment.** This is a record template/procedure, not
a flashing script. Parent AVB authentication and fresh actual LP/snapshot
validation are unresolved. Do not use bootloader unlock, disabled verification,
arbitrary resizing, metadata/userdata wipes, or CRC/signature bypass as a test
precondition. No test in this document has been performed for these new modules.

## Exact scope and artifacts

Start from the qualified ColorOS `CPH2745_17.0.0.102(EX01)` 6.12.114 kernel and
stock system/vendor DLKM. Preserve the source/toolchain/private-layout and
module-provider manifests. The qualified boot remains unchanged, SHA256
`1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3`.
Stock vendor-DLKM rollback SHA256:
`82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47`.

The integrated **offline-only** four-provider vendor-DLKM image is
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/security/offline-vendor-dlkm-run06/vendor_dlkm.img`,
SHA256 `b8a5385cb563188640395b4117681b782f8e3a97fcd914d8ce6a34af0e6e62db`.
Do not confuse its valid local hashtree/FEC with a parent-vbmeta-authorized
image. [Packaging evidence](coloros17-four-provider-offline-package.md) names
every replacement and exact unchanged file/metadata check.

For initial physical isolation, prefer independently rebuilt/revalidated
stages: A RMNET only; B add QCE/QCrypto together; C add WCD. Those stage-specific
partition images have not been produced or qualified here and must not be
created through ad-hoc shared-block filesystem edits. Every new image requires
its own artifact/metadata/oracle/AVB/capacity gates and SHA manifest. Do not
substitute quarantined WLAN/GPU/camera/video/DSP modules into any stage.

## Preflight and controlled write boundary

1. Obtain explicit authorization for a named stage and exact payload hashes,
   only after a device-specific authenticated AVB path exists. Record exactly
   which signed parent component would have to change, if any; no assumption
   that unlocked or orange state authorizes a modified hashtree is acceptable.
2. Freshly capture firmware/device identity, active slot, corresponding inactive
   slot, logical partition names/sizes, super group/extents and snapshot/update
   status. Refuse writes while snapshots are active/merging or mappings unknown.
   Run raw LP checks for both relevant metadata slots; the current offline
   capacity tool is selected-slot-only and is not itself deployment permission.
3. Rehash all rollback images and exact candidate. Validate all embedded files,
   original module paths/loading metadata, filesystem, SELinux, signature/
   provider policy and parent AVB relationship. Prepare a tested no-data-wipe
   restoration path before changing the device.
4. Use the separately reviewed fastbootd/logical-partition path, only for the
   minimum explicitly named active-slot components. Back them up first;
   preserve inactive-slot and dynamic metadata. Read back the complete actual
   written payload and compare exact SHA before boot. Stop on transport loss
   or ambiguous slot/mapping state rather than guessing another write target.

## Independent result matrix

Each row must record stage, exact hashes, boot number/uptime, actions, observed
result, associated dmesg/logcat/pstore filenames and any first failure. PASS
requires observation; service initialization alone is not hardware PASS.

| ID | Test | Required observations | Current result |
|---|---|---|---|
| P01 | Kernel and module identity | Exact uname/Image; four intended provider hashes/paths; no competing copies | NOT TESTED |
| P02 | Module loader | No missing symbols, CRC/version, namespace, protected export, signing or dependency failures | NOT TESTED |
| P03 | Android boot | sys.boot_completed=1; no init_user0_failed/bootstrap-apexd failure | NOT TESTED |
| P04 | FBE/user0 | PIN unlock; user0 RUNNING_UNLOCKED; CE and DE accessible | NOT TESTED |
| P05 | Storage | /data F2FS+inlinecrypt; no fscrypt/blk-crypto/UFS/keyslot errors | NOT TESTED |
| P06 | KeyMint and TWRP | Metadata mount, mapper creation, PIN, CE/DE; no unwrap regression/SIGABRT | NOT TESTED |
| P07 | RMNET/LTE/5G | Data transfer, RAT/network transitions, voice call; no aggregation/parse/drop regression | NOT TESTED |
| P08 | Tethering/VPN | Wi-Fi and USB tethering, VPN connect/reconnect and bidirectional traffic | NOT TESTED |
| P09 | WLAN | Connect, transfer, disconnect/reconnect, screen-off wake; no firmware SSR | NOT TESTED |
| P10 | Bluetooth | Pair/reconnect watch/headset; audio/call transport; no core/vendor errors | NOT TESTED |
| P11 | GPU/display/touch | App/compositor rendering, screen cycles, touch; no GEM/refcount/GPU faults | NOT TESTED |
| P12 | Camera/video | Photo, focus, video recording and playback; no HAL, OIS, HFI or SSR errors | NOT TESTED |
| P13 | Audio/DSP | Speaker/headset playback, microphone, call; repeated routes; no PRM clock timeout/regression | NOT TESTED |
| P14 | Fingerprint/NFC | Authentication and actual NFC operation with available test hardware | NOT TESTED |
| P15 | USB-C/OTG | MTP, USB data/tethering and removable block read/write/eject/reconnect | NOT TESTED |
| P16 | Charging/thermals | Plug/unplug, normal negotiated charging, temperatures under repeatable load | NOT TESTED |
| P17 | Suspend/resume | Disconnect USB/ADB completely, screen off/idle, manual wake, reconnect and inspect suspend/wakeup stats | NOT TESTED |
| P18 | Reboot cycles | Repeated cold/warm boots and post-reboot PIN with same exact stage artifact | NOT TESTED |
| P19 | Watchdog 0–240s | Preserve all scans beyond former121/183s points; distinguish idle waits from active SCM/QPACE/HFI stalls | NOT TESTED |
| P20 | Extended runtime | 10–15min idle/use plus a repeatable transfer/audio/load window, no unexplained hung tasks | NOT TESTED |
| P21 | Fatal/error scan | No panic, oops, BUG, unexplained WARN, RCU/lockup/refcount/UAF, storage errors or new fatal pstore | NOT TESTED |

Do not provoke real hardware crypto/DSP/firmware faults merely to test an error
path unless that separate fault-injection procedure and recovery are reviewed
and authorized. Host sanitizer tests do not establish IRQ/DMA/firmware race
coverage. Record remaining NOT TESTED rows rather than assuming success.

## Failure/rollback

Collect safe first-failure logs before any recovery action; never overwrite
pstore evidence by repeatedly booting the same failed candidate for more data.
Stop that stage on FBE, module, security or functional failure. Restore the exact
same-firmware qualified affected raw components using the reviewed slot/path;
read back all restored bytes, verify their hashes, then verify Android,
RUNNING_UNLOCKED and F2FS+inlinecrypt. The complete rollback manifest is in
[the main report](coloros17-full-qualcomm-modernization.md). Stock init_boot,
vendor_boot, system-DLKM, DTBO and encryption metadata do not become change
targets simply because the vendor-DLKM experiment fails.

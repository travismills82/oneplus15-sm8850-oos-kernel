# ColorOS 17.0.0.102 experimental driver test and rollback plan

This is a plan, not an executed flash procedure. No device is connected to
ADB during this audit. It does not authorize flashing, resizing a logical
partition, modifying vbmeta or disabling verification.

## Immutable rollback set

The independent backup is
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-baseline-20261030/`.
Verify its `baseline-sha256.txt` before any future device session.

| Payload | SHA256 |
|---|---|
| Qualified boot | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| Original system_dlkm | `3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a` |
| Original vendor_dlkm | `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47` |
| Original vendor_boot | `d0805a3cf5007294f2976f3f579c31e120b2d98e600a170f93dfec78b6baf000` |

Use the full original firmware images, not extracted filesystem-only
`*-verity.img` files. Preserve the previously qualified boot bytes; do not
rebuild them merely to label this experiment.

## Gates before asking for a physical canary

1. Freeze the exact candidate module, source commits, effective DDK config,
   compiler hashes and dependency SDK inputs. Include every direct stock
   consumer and private callback/layout constraint.
2. Run the production-realistic hybrid module oracle with replacements at
   their original partition/path. Do not substitute SDK-only dependency
   builds for retained stock providers. Validate actual unsigned-import and
   protected-export policy, not just CRCs.
3. Resolve the signed-parent AVB issue described in
   [the packaging gates](coloros17-build-and-packaging-gates.md). The original
   top-level vbmeta directly authenticates both DLKM roots. An unsigned
   local footer alone does not make a modified filesystem acceptable. Do not
   disable AVB, alter rollback indexes or assert an unknown bootloader state.
4. Validate image filesystem format, labels, ownership, capacities, complete
   module metadata and the approved verification chain. No vendor.img is
   needed for the proposed RMNET-only source change.
5. Obtain separate explicit authorization for the exact payload/partition set.

Until gate 3 has a legitimate solution, there is no installable DLKM
candidate and no reason to write any partition. Source/build experiments can
continue independently.

## Fresh device preflight

Confirm the exact firmware, device, transport, running kernel, root access,
active slot, boot health and user0 state. Do not reuse a prior slot letter.
Resolve boot and logical DLKM mappings and record their capacities. Collect
the currently installed partition hashes and a fresh hash-verified backup.
Compare supporting partitions with the rollback generation; stop on an
unexplained mismatch or ambiguous transport.

The stock module inventory has early-boot/recovery copies as well as vendor
copies. An RMNET core replacement belongs only to the original vendor_dlkm
path. Do not write vendor_boot or system_dlkm merely because a generic Canoe
distribution emits them. Preserve inactive-slot rollback and do not resize
dynamic partitions without a separately reviewed need and authorization.

## One-variable test matrix

If a matched image is eventually approved, replace only the justified module
group, verify full read-back bytes and collect evidence for each gate:

| Area | Required observation | Current experimental status |
|---|---|---|
| Startup/FBE | boot complete, user0 unlocked, mapper, CE/DE, F2FS+inlinecrypt | NOT TESTED |
| Recovery | metadata/PIN/CE/DE decryption using established recovery procedure | NOT TESTED |
| Module loading | actual intended provider, signatures, CRCs, no loader errors | NOT TESTED |
| RMNET | LTE/5G data, voice, reconnect and airplane-mode cycle | NOT TESTED |
| Networking | Wi-Fi, VPN, tethering and network transitions | NOT TESTED |
| Other hardware | Bluetooth, NFC, camera/video, audio, display/touch, fingerprint | NOT TESTED |
| USB/power | OTG, charging/thermals, USB-disconnected suspend/resume | NOT TESTED |
| Kernel health | dmesg/logcat/pstore, RCU/locking/refcount, watchdog traces | NOT TESTED |

Observe at least both earlier 121/183-second watchdog windows and a longer
idle/runtime interval. Do not classify ADCI, QPACE or HFI activity by thread
name alone; preserve stack-specific idle/active-stall discrimination.
Malformed MAP tests on a host model are not a substitute for real RMNET
traffic, firmware interaction, race testing or this physical matrix.

## Rollback

On a failed canary, preserve diagnostics when safe, then restore the exact
qualified boot and only the partitions actually changed, using the resolved
slot/mapping and an approved verified-boot chain. Verify complete restored
read-back hashes. Confirm Android, user0, F2FS/inlinecrypt and connectivity.
Never erase userdata/metadata, regenerate encryption keys, change radio or
persist, or disable security enforcement as a recovery shortcut.

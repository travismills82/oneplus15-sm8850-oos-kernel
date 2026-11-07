# Linux 6.12.116 boot-only physical canary — 2026-11-07

**Initial Android/FBE, owner-reported functional hardware checks and final
reconnect sweep PASS. Extended power/stability qualification remains pending;
known warnings and enforced GKI FAIL remain disclosed. Not stable-release
qualification.**

The owner made the phone available to test the prepared .116 candidate under
the standing controlled physical-test authorization. No rebuild or repackage
occurred. Only active boot_a was written, followed by one normal Android reboot.
The passing candidate is left installed; no rollback or slot change occurred.

## Identity and read-back

- Source: `baba3e32875ab06ed6d53127cbdbced08e980ce4`, branch
  `experiment/coloros17-linux116-boot-only`.
- Raw `/proc/version` confirms
  `6.12.116-android16-6-o-gbaba3e32875a-4k`, not spoofed uname values.
- Physical firmware manifest: `CPH2747_17.0.0.102(EX01)`.
- Raw bootconfig confirms slot A, unlocked/ORANGE. Overridden Android
  properties are not used as bootloader-state evidence.
- Exact full 100,663,296-byte boot read-back SHA256:
  `0c39caaa3c2095b834f2687a10ddce8e445b32d3dfc9cca363edb708c5be288a`.
- Running config SHA256:
  `92002eb1181dee6400f8173341767316a7a488c6ab675cbd248efbfdd8b6e1d7`,
  byte-identical to the frozen .116 build.
- Fresh original boot_a backup, verified on device and host:
  `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc`.

Full evidence, images, guarded procedure and restoration commands:
`/home/travis/Android/kernel-test-backups/coloros17-linux116-canary-20261107-JzEID9/RESULT.md`.
The authoritative rollback is that directory's `boot_a-before.img`.

## Safety gates and preserved partitions

Fresh firmware, slot/mapping/capacity, raw unlocked state, completed/unlocked
Android, F2FS inlinecrypt and empty OTA/snapshot checks passed. Actual super
metadata was captured. Frozen candidate hash/size and offline AVB verification
passed. The guarded dry-run, device full read-back and independent host full
read-back passed before reboot.

The first read-only guard stopped on an outdated recovery-A expectation.
No partition was written at that point. The current recovery was positively
identified by exact hash as the Nov 6 installed/booted/decrypted/read-back-tested
GLES TWRP ed9b414 build. That new provenance was checked before proceeding;
recovery B remains unchanged. Both recoveries were preserved.

Thirteen protected companion hashes match before/after flash and after boot:
boot_b, both init_boot/vbmeta/recovery/vendor_boot/DTBO slots and active stock
system_dlkm/vendor_dlkm. The current root image is **KernelSU-patched
init_boot**, not Magisk. Neither init_boot is modified. No verification bypass,
key enrollment, data/metadata wipe, driver replacement or DLKM write occurred.

## Runtime results

| Gate | Result |
|---|---|
| Android startup / boot_completed | PASS |
| user0 RUNNING_UNLOCKED | PASS |
| F2FS /data + inlinecrypt | PASS |
| CE/DE paths | Accessible; user contents not inspected |
| SELinux | Enforcing |
| Stock DLKM/verity environment | Original images/parent unchanged; verity mappings mounted, no observed verity error |
| KernelSU | Root works; ksud reports 3.3.1-36-g28db74f6, UAPI 6 |
| SUSFS | v2.3.0 and expected enabled features available |
| Installed Droidspaces checker | PASS, requirements_met=1 |
| Full Linux distro/container session | NOT TESTED |
| Stock providers | 691 modules loaded, including KGSL/GEAS, Bluetooth/PPP/RFKILL/SLHC, RMNET and Rust Binder |
| Graphics service | Adreno 840 hardware GLES 3.2 reported; sustained rendering not tested |
| Wi-Fi traffic | 3/3 ping responses, no packet loss |
| Bluetooth | Service and existing watch connection observed |
| NFC | Service on, no external tag test |
| Wi-Fi/Bluetooth hands-on operation | PASS, owner reported |
| Mobile-data traffic | PASS, owner reported |
| Camera/photo/video | PASS, owner reported |
| Audio/microphone | PASS, owner reported |
| Fingerprint | PASS, owner reported |
| OTG | PASS, owner reported |
| USB-disconnected controlled idle/deep suspend | NOT QUALIFIED; suspend cycles observed, see below |
| Recovery/PIN decryption | NOT REPEATED in this .116 cycle |

After the hands-on window the owner explicitly confirmed completion of the
listed Wi-Fi/Bluetooth, mobile-data, camera/video, audio/microphone, fingerprint
and OTG checks: "finished testing everything is working well." These are
recorded as owner-reported functional PASS, separately from automated evidence.
They do not establish call, NFC-tag, TWRP, sustained gaming, container-session
or controlled screen-off power qualification that was not explicitly tested.

## Logs and limitations

Captured dmesg covers startup through 282.690979 seconds, passing the prior
121/183-second warning boundaries. The full Android/user0 check passed at
285.69 seconds; a later uptime probe returned 306.35 seconds on the same boot.
No panic/Oops/BUG, symbol/CRC/load failure,
CFI failure, RCU stall, lockup, hung-task report, GPU fault, UFS I/O-error or
verity-error signature is observed in that retained interval. Pstore was empty.

Two WARN roots match the prior .115 boot evidence: unprivileged-eBPF/BHB policy
at 4.214108 seconds and unchanged stock HBP `frame_put` field-spanning memcpy
at 5.008615 seconds (`hbp_frame.c:88`, `+0x36c/0x44c`). Neither is suppressed,
claimed repaired or omitted from the result.

ADCI, ZRAM and HFI still occupy the same previously documented stock service
wait paths. ADCI is in `__scm_smc_call` through `adci_fn`, not `wait_to_die`;
the existing source documents supported `adciAccept` as non-returning. The
inherited detector inspects task/wait channel, not live queue/transaction state;
zero reports do not independently prove Secure World/QPACE/HFI transaction
health. The global detector remains enabled, runtime timeout 60 seconds, and
no detector code or sysctl was changed by this test.

### Final reconnect sweep after hardware testing

USB/ADB returned. Fresh checks at 1,358.90 seconds (about 22.6 minutes uptime)
confirm the same .116 kernel, boot hash, user0 RUNNING_UNLOCKED, F2FS inlinecrypt,
SELinux Enforcing and unchanged thirteen protected partition hashes. Pstore is
empty and the global hung-task timeout remains 60 seconds. No installed image,
slot, module policy or system setting was changed during the follow-up.

Retained post-hardware dmesg covers kernel timestamps 391.196155–1114.329759;
the boot ring has wrapped. Together with the earlier 0–282.690979 capture this
does not prove every intervening log message was retained. Kernel printk time
and /proc/uptime also differ across suspend; they are not equated. The final
retained scan contains zero panic/Oops/BUG, module-version/symbol, CFI, RCU-stall,
lockup, hung-task, GPU-fault, UFS I/O-error or verity-error signatures. The
captured crypto log has no tracked init_user0_failed/DeviceUpgradeKey/
INVALID_ARGUMENT/SIGABRT regression signature. This remains a bounded scan.

The later ring contains **60 printed timekeeping WARN records, 36 distinct
timestamp/CPU/header records across 12 timestamp groups**, at 1035.682344–
1071.020531. They all identify `timekeeping.c:747 ktime_get+0xdc/0x118`.
The path is `qcom_lpm::cluster_power_cb -> ktime_get`, during genpd/PSCI
s2idle domain power-off/on. `ktime_get()` warns when timekeeping is suspended.
The stock qcom_lpm module build ID is
`0b93f8f2df6628f01bb0ed70331e69b4a007960e`.

Classification: **PRE-EXISTING warning root, not repaired**. The fresh .115
preflight `dmesg-before.txt` contains the exact warning line, `ktime_get` offset,
same qcom_lpm build ID and `cluster_power_cb -> genpd_sync_power_on ->
dev_pm_genpd_resume -> __psci_enter_domain_idle_state` stack. This is direct
same-device baseline evidence, not inference from an older report or from
successful boot. No .115-to-.116 change exists in timekeeping, freezer,
alarmtimer, genpd or Qualcomm driver sources. There is no rate comparison:
baseline and candidate observation windows/workloads differ. Warnings were
not suppressed; the follow-up is not described as warning-free.

Suspend counters at the reconnect sample were 54 successes and 39 failed/
aborted attempts: 35 freezer-stage and four suspend-stage failures, with zero
resume/early-resume/noirq-resume failures. The last recorded device was
`alarmtimer.2.auto`, errno -16 (`EBUSY`). The retained freezer messages say
"aborted" within 0.04–0.323 seconds, not a 20-second freeze timeout. The
unchanged freezer uses that wording for pending-wakeup interruption. The
unchanged alarmtimer can intentionally return EBUSY for an alarm due within
two seconds; an EBUSY counter is not automatically a broken driver. The full
35/4 attempt history is not retained, so not every attempt is independently
classified. These observations do not establish controlled unplugged idle,
deep sleep, suspend efficiency or zero power-management defects.

Evidence additions under the host backup directory: `after-hardware-status.txt`,
`dmesg-after-hardware.txt`, `kernel-scan-final.json`,
`timekeeping-baseline-comparison.json`, `suspend-stats-after-hardware.txt` and
`logcat-after-hardware.txt`. The prior empty capture attempted during USB
disconnection is rejected, not counted as a clean log.

Disposition: **functional .116 boot-only canary PASS with disclosed inherited
warnings; controlled power/extended stability and unchanged GKI limitations
remain**. No new candidate-specific fatal/module failure is observed in the
retained evidence. This is not approval to publish or a claim of stable/security
completeness.

Offline build, strict symbol-list, ABI generation, exact 1,070-module contract
and provider checks from the [integration report](coloros1700102-linux-6.12.116-integration-20261107.md)
apply to these exact bytes. Enforced GKI remains **FAIL with 45 unchanged
owners**, not overall PASS. No ABI reference was updated and no security-
completeness or universal-device qualification claim is made.

During physical testing no commit, push, merge or publication occurred, and
main remained unchanged. The owner subsequently explicitly authorized a source
push to main. That separate source-only action preserves the four upstream
commits and adds this validation/README documentation without rebuilding or
altering the tested images. It is not a stable-release/GKI waiver or release
publication authorization.
The candidate stays installed for testing, with exact .115 boot-only rollback
staged. Do not restore or alter init_boot, switch slots, or touch the stock
DLKM/AVB environment as a substitute for restoring the verified boot backup.

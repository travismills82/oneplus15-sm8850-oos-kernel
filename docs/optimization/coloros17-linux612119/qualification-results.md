# Exact offline and device qualification

The production phone remains on `97d4684f2e096`; verification was read-only.
Neither experimental image was installed. Keep Google GKI and the device
ABI comparison separate.

## A (physically tested) and B1 (one efficiency change)

- A is `6.12.119-android16-6-o-g97d4684f2e09-4k`, slot A. The full running
  boot hash is `aabd0c59d8cb6c9b27448c4c947fda90471e1fbb194837803f2e5b1b824448e4`.
  Exact prior physical read-back, stock verity roots, user0/FBE and hardware
  canary records are preserved.
- B1 is `6c266e436449fc745971af7fc0656f19e3256669`, a single patch that
  corrects the shadow-node memory budget for `nokmem`.
  `out/coloros17001201-battery-evidence/candidate-6c266e436449f/boot.img`
  SHA256 `df861762c62d9b0ee51ffb2759887e07a69b1e883f1d9bbc7b47fff738d3279a`
  (100,663,296 bytes). The manifest binds source, raw Image, boot, config,
  symvers and exact rollback hashes.

| Gate | B1 result |
| --- | --- |
| Canoe perf common build, strict KMI, common ABI generation | PASS; unchanged generated config including all security options. |
| Independent ColorOS ABI | PASS/exit 0 using the corrected STG extractor and the frozen .119 compiled baseline; empty report. |
| Official enforced GKI | FAIL/exit 4; same small report as the physically tested .119. Reference untouched. |
| Linked and rebuilt export versions | All 10,173 vmlinux and 10,678 combined export records equal the baseline. All stock arm_smmu CRCs preserved. |
| Stock module and provider oracle | PASS, freshly exercised: 1,069 records, 1,031 compatible, 38 dormant, 60,835 hard imports, 61,910 CRC records; no CRC, namespace, provider or load-policy failures. |
| Compiled private layouts and KCFI | PASS: 39 compiled layout roots (one absent in both), all 2,533 named KCFI IDs unchanged, plus eight linked Rust/C prefixes. |
| ARM64 offline functional tests | PASS: namespaces, cgroup v2, seccomp, IPC, scratch ext4/F2FS remounts and OverlayFS. Official Droidspaces v7.2.0 requirements check and an Alpine ARM64 namespace/start/file/stop workload passed in QEMU with `cgroup.memory=nosocket,nokmem`. |
| Host checks | The extracted production C shrinker passes configured and non-memcg cases; the original fails the targeted nokmem assertion. Telemetry interval sanity tests passed. Codec screening is not a physical power test. |
| Offline AVB | PASS; exact raw Image re-extracted from Android v4 boot; preserved .201 AVB properties and development signer with flags zero; no companion artifact generated. |
| Physical testing and battery gains | NOT TESTED. Neither a deep-sleep improvement nor an energy saving on the handset is claimed. |

The existing independent reference in
`out/coloros17001201-linux119-preflight/kbuild/offline-97d4684f2e096-20261123-112832/filtered.stg`
has SHA256 `c2e8de88cf815c9521b0944fee65021449acabd7d5e1235b5f69d7e1e743f612`.
This report does not authorize overwriting either the device or GKI reference.
The exact host build, tool, runtime test and ABI receipts are stored under
`out/coloros17001201-battery-evidence/qualification-6c266e436449f`.

## B2 (additionally fixes block plug store ordering)

Built from source `a9059fb3d6d64434c533383ddf5dbac270995d5d` with
`b452ac5e72b64` as a separate stability patch. `boot.img` is in
`out/coloros17001201-battery-evidence/candidate-a9059fb3d6d64/`:

- Kernel release: `6.12.119-android16-6-o-ga9059fb3d6d6-4k`.
- Boot SHA256: `29002f26c6b5bb12dd59dd1887b4b24dbcf31297bd2118277eaed93dfd90946e`;
  exactly 100,663,296 bytes.
- Raw Image SHA256: `6f35071275b004435febe7f4bfb4e0edc90e5a30df0956fd3e39d8b1433843c5`.
- Config SHA256: `1e7ab7814e8b12a852aa2e0365998b95ba3f5a548852cb910cfec12460f8b9a3`.
  Module.symvers SHA256: `f340255d464bb872fbb92a1891256d59495f42db82738ce6e9833acd551f04ad`.

B2 repeated the common build, strict KMI, ABI generation, private-layout
and KCFI tests, all 1,031 active stock modules and 38 dormant records,
provider policy, ARM64 regression, actual Droidspaces/Alpine QEMU workload,
and offline AVB checks; all passed. Independent ColorOS STG again returned
exit 0 with an empty report. The stock module population was re-audited for
this exact compiled image, not copied from a prior result.

The ARM64 disassembly confirms the previous flag-clear store at `+0x70`
still executes, and the plug-clear store now follows at `+0x74`, not at
`+0x68` before the flag. This is a stability check, not a battery result.
`blk_finish_plug` retains CRC `0x1c3364a5`. The actual **enforced GKI**
result is **FAIL/exit 4**. Its uncorrected report adds one block for debug
type recovery of `void blk_finish_plug(struct blk_plug*)` against the
untyped checked-in reference. Running the corrected extractor against
that same unchanged Google reference produces the exact prior corrected
report. No new ColorOS runtime ABI difference or reference edit occurred.

Run the [A/B plan](battery-test-plan.md) with B1 first to isolate reclaim
effects. B2 can be tested separately for block stability and final
regressions. Both are offline experiments, not release or flash approval.

# Combined Bluetooth .046 and NXP NFC .102 static validation — 2026-08-22

## Result

**PASS — combined vendor-DLKM candidate is statically valid.**

This candidate starts from the physically qualified WLAN `.053` vendor-DLKM
and changes exactly four module payloads: `btpower.ko`, `bt_fm_swr.ko`,
`btfm_slim_codec.ko`, and `nxp-nci.ko`. Boot, system-DLKM, vendor boot, VBMeta,
all WLAN modules, all 27 stock cellular modules, and all unrelated vendor
modules remain unchanged.

The combined image is a core-qualification candidate, not a final release.
Bluetooth peripheral tests and NFC tag/payment tests remain explicitly open.

## Frozen kernel contract

| Item | Qualified value | Combined result |
| --- | --- | --- |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` | unchanged |
| Boot SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` | reused exactly |
| System-DLKM SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` | reused exactly |
| `.config` SHA-256 | `b53d48b303059adb49a8dbe457145a4b7523a77fae621ea8d9e7b0b727e1615b` | identical |
| `Module.symvers` SHA-256 | `de57709f3de38afb3e266481da09433687979ffb88ee607bda93ac4732dd7e0b` | identical |
| `System.map` SHA-256 | `4f8ef4feb7c71abc18def8db341701e05b1ff7c0e8c8b50de7da48ef100f011a` | identical |
| Functional `Image` differences | none | PASS |
| Functional `vmlinux` differences | none | PASS |
| ABI diff | empty | PASS |
| KMI symbol checks | unchanged | PASS |

The fail-closed kernel-contract guard passed before the external DDK targets
were accepted. Bluetooth and NFC source identities are package metadata and do
not change the frozen g6744 kernel contract.

## Build validation

The following clean validation completed successfully:

```text
tools/build-controlled-v1-external-ddk.sh --clean ...
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel build \
  //common:kernel_aarch64_abi \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //common:kernel_aarch64_abi_diff
```

The external-DDK build reported `KERNEL CONTRACT GUARD PASS` and
`CONTROLLED EXTERNAL DDK BUILD PASS`. The complete Canoe dist target also
completed successfully. Existing non-fatal Kleaf deprecation, DTS, and broad
dist depmod warnings were not introduced by the four-module candidate; the
actual proposed 436-module runtime graph was validated separately and has zero
unresolved imports.

## Combined runtime graph

| Gate | Result |
| --- | --- |
| Vendor modules | 436 |
| Intended replacements | 4 |
| Unexpected replacements | 0 |
| Exact stock cellular modules | 27 |
| Cellular hash failures | 0 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature failures | 0 |
| Vermagic failures | 0 |
| Duplicate module names | 0 |
| External signed-provider edges | 0 |

`bt_fm_swr` imports `swr_read` with CRC `0x5e6b9976`; it resolves to the
retained `swr_dlkm` provider with the same CRC.

The `.102` `nxp_nci` module has 81 imports and all resolve with matching CRCs.
The actual controller is a character-device/I2C transport and does not import
Linux NFC/NCI core APIs directly; its providers are `vmlinux`, retained
`smcinvoke_dlkm`, retained `qcom_ipc_logging`, and retained `pinctrl_msm`.
Therefore the built-in NFC core contract is unchanged rather than linked by a
direct module import edge.

## Delivery contracts

- system-DLKM `modules.load`: 46 entries;
- `wwan.ko`: entry 21;
- missing entries: 0;
- stale built-in entries: 0;
- `cfg80211.ko` precedes Peach-v2 in vendor load policy;
- vendor image filesystem check: PASS;
- partition-local AVB hashtree/FEC/footer verification: PASS; and
- image read-back of every replacement module: exact SHA-256 match.

## Candidate identity

```text
vendor_dlkm.img
SHA256 3ed964f345e6f5040c70ef7c0c083c1fc4bab536b6a522ca83c61b20be032ed4
size   143986688 bytes
```

Subsystem identity:

```text
kernel=6.12.23-android16-5-o-g6744a3f6bcf4-4k
wlan=.053
bluetooth_vendor=.046
nfc_vendor=.102
cellular=stock OOS 16.0.9.400(EX01)
signing=controlled-v1
bluetooth_qualification=CORE_PASS_OPTIONAL_EQUIPMENT_PENDING
nfc_qualification=CORE_PASS_TAG_PAYMENT_PENDING
```

Physical status at this point: **NOT YET TESTED AS A COMBINED STACK**.

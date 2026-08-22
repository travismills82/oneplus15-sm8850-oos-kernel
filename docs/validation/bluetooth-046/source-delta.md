# Bluetooth vendor platform .031 to .046 source delta

## Scope and provenance

- Frozen baseline branch: `feature/controlled-v1-wlan053`
- Frozen baseline commit: `e7444f14cbbd587e528fab386ad886ee474138c6`
- Experiment branch: `experiment/bluetooth-vendor-046`
- OnePlus 15 `.031` source: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- Official newer same-SM8850 `.046` source: `d447f713d6403f707a2910383495f4ada98cfa4d`
- Relevant OnePlus synchronization commit: `bc8d91d1e146be96d2e27bebe8f753f82bdebeee`
- Old tag: `AU_TECHPACK_BTFM.LA.2.0.R1.00.00.00.000.031`
- New tag: `AU_TECHPACK_BTFM.LA.2.0.R1.00.00.00.000.046`

The old `bt-kernel` tree in the controlled repository was byte-identical to
the official `.031` tree before this experiment. The four imported source
files are byte-identical to their official `.046` blobs. No 15T branch was
merged.

## Active Canoe path

Runtime `/proc/modules`, the frozen vendor-DLKM `modules.load`, generated
`modules.dep`, source DDK rules, and boot dmesg prove the normal path:

```text
qcom,peach-bt device tree
        |
        v
     btpower <---------- cnss_utils <---------- smem_mailbox
        |
        +------ btfm_slim_codec ------ slimbus
        |               |
        |               +------------- btfmcodec ---- Qualcomm RPMSG/remoteproc
        |
        +------ bt_fm_swr ------------- swr_dlkm
                        |
                        +------------- btfmcodec ---- Qualcomm RPMSG/remoteproc
```

`radio_i2c_rtc6226_qca` is also loaded from the Bluetooth package, but its
source is unchanged in `.046`. The Linux Bluetooth core, HCI UART/QCA path,
RFKILL, RFCOMM, and HIDP remain built into the controlled Image and are out of
scope.

The complete 13-module loaded closure and frozen hashes are recorded in
`current-bluetooth-closure.tsv`.

## Minimal replacement decision

| Module | Decision | Evidence |
| --- | --- | --- |
| `btpower` | REPLACE WITH .046 | Active Peach power/reset/RFKILL/HAL module; source changed. |
| `bt_fm_swr` | REPLACE WITH .046 | Loaded normal-boot SoundWire endpoint; source changed. |
| `btfm_slim_codec` | REPLACE WITH .046 | Loaded normal-boot Slimbus endpoint; two compiled source files changed. |
| `btfmcodec` | KEEP EXISTING | Loaded shared endpoint coordinator, but source did not change. |
| `radio_i2c_rtc6226_qca` | KEEP EXISTING | Loaded FM transport, but source did not change. |
| CNSS/SMEM/RPMSG/remoteproc/audio providers | KEEP EXISTING | No BTFM `.046` source change and no new direct dependency. |
| `spi_cnss_proto` | NOT APPLICABLE | Not present in the qualified vendor image and not loaded on Canoe. |
| `thqspi_proto` | DORMANT / NOT APPLICABLE | Build definition exists, but no qualified image output or runtime load. |

This is a three-module runtime delta. WLAN053 modules and the exact-stock
27-module IPA/GSI/RMNET/data closure are not part of it.

## Active code changes

### `btpower.c`: race/liveness bugfix worth testing

`.046` adds `pid_alive()` checks before BT and UWB workers signal the HAL task,
and extracts the UWB panic path into a `__noreturn` helper. The liveness guard
is relevant to the active Canoe `btpower` path and can avoid signalling a task
that has exited.

This is deliberately **not** classified as a proven complete UAF/security fix:
`btpower_register_client()` stores `get_current()` and the audited source does
not pair that storage with `get_task_struct()`/`put_task_struct()`. The new
guard improves the race behavior but does not itself prove lifetime ownership.

### `btfm_swr.c`: SoundWire power/audio correctness

Before opening the first BT SoundWire port, `.046` reads `SWRS_SCP_STATUS`.
This runs in the loaded `bt_fm_swr` module and is relevant to A2DP/HFP audio
transport qualification.

### `btfm_slim_hw_interface.c`: LHDC sampling-rate correction

The `.046` Slimbus endpoint handles LHDC like LDAC and aptX Adaptive when
doubling 44.1/48 kHz transport sample rates. This is active only when that
codec/transport path is selected, but it is device-independent audio
correctness code.

### `btfm_slim_codec.c`: build/API compatibility

`.046` uses `snd_soc_dai::symmetric_rate` on Linux 6.13 or newer and preserves
the existing `dai->rate` path below 6.13. The current 6.12.23 runtime behavior
is unchanged; importing this file keeps the `.046` module source coherent.

## Complete changed-path classification

| Changed path | Classification | Canoe action |
| --- | --- | --- |
| `bt-devicetree/Kbuild` | BUILD ONLY / DEVICE TREE | DROP; only additional board DT targets. |
| `bt-devicetree/alor-peach-bt.dtsi` | DEVICE-SPECIFIC | DROP; Alor pin power-source change is not Canoe. |
| `bt-devicetree/alor-wcn7750-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/canoe-cdp-kiwi-no-l6k.dts` | DEVICE-SPECIFIC | DROP; inactive Kiwi/no-L6K target. |
| `bt-devicetree/canoe-cdp-peach-no-l6k.dts` | DEVICE-SPECIFIC | DROP; new board target, not CPH2747. |
| `bt-devicetree/canoe-kiwi-bt.dts` | NOT USED BY CANOE PEACH | DROP. |
| `bt-devicetree/canoe-peach-bt.dts` | DEVICE TREE / BOARD IDS | DROP; only adds other MSM IDs, not a driver contract. |
| `bt-devicetree/canoe-wcn786x-bt.dts` | NOT USED BY CANOE PEACH | DROP. |
| `bt-devicetree/canoe-wcn786x-no-l6k.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/kera-qca6750-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/kera-qca6750-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/kera-wcn7750-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/kera-wcn7750-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/tuna-kiwi-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/tuna-kiwi-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/tuna-wcn7750-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/tuna-wcn7750-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/x1e80100-kiwi-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/x1e80100-kiwi-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/x1p42100-kiwi-bt.dts` | DEVICE-SPECIFIC | DROP. |
| `bt-devicetree/x1p42100-kiwi-bt.dtsi` | DEVICE-SPECIFIC | DROP. |
| `bt-kernel/BUILD.bazel` | BUILD ONLY | DROP; adds Parrot target wiring, not Canoe. |
| `bt-kernel/pwr/btpower.c` | RACE/LIFETIME; POWER/RESET | IMPORT; active Canoe module. |
| `bt-kernel/slimbus/btfm_slim_codec.c` | BUILD ONLY / API COMPATIBILITY | IMPORT as part of coherent active module. |
| `bt-kernel/slimbus/btfm_slim_hw_interface.c` | BUG FIX / AUDIO | IMPORT; active Canoe module. |
| `bt-kernel/soundwire/btfm_swr.c` | BUG FIX / POWER/AUDIO | IMPORT; active Canoe module. |
| `bt-kernel/spi/include/spi_cnss.h` | NOT USED BY CANOE | DROP; SPI-CNSS module absent from qualified image/runtime. |
| `bt-kernel/spi/spi_cnss_proto.c` | MEMORY-SAFETY-SHAPED HARDENING; NOT USED BY CANOE | DROP; no runtime/binding evidence. |
| `bt-kernel/spi/spi_cnss_proto.h` | NOT USED BY CANOE | DROP. |
| `bt-kernel/target.bzl` | BUILD ONLY | DROP; adds Parrot module set. |
| `bt-kernel/target_variants.bzl` | BUILD ONLY | DROP; adds Parrot variant. |

## Device-tree contract

The authoritative CPH2747 source remains `canoe-peach-bt.dtsi` with
`compatible = "qcom,peach-bt"` and the existing Canoe pin/regulator data. The
`.046` `btpower` driver retains the same `qcom,peach-bt` match and introduces
no required Peach property or compatible string. No device-tree import is
needed.

## Release-identity checkpoint

The controlled-v1 release contract deliberately derives the Kleaf SCMVERSION
from the newest committed change in `kernel_platform`, `vendor`, and the
signing/source-ID guard inputs. Commit
`8906fd47be43616ee8ed532ae571ecbe30dced49` changes `vendor/`, so the supported
build path derives source identity `8906fd47be43`, not the physically qualified
WLAN053 identity `6744a3f6bcf4`.

Consequently, newly built and normally signed `.046` modules cannot honestly
be packaged as a vendor-only drop against the old boot release. Doing so would
require forcing the old SCM identity or bypassing the fail-closed release
guard. Both would violate the controlled signing and vermagic contract.

The safe next candidate therefore requires a new matching controlled-v1
`boot.img` and matching custom module outputs. Per the experiment's stop rule,
no broader candidate has been built or flashed pending explicit approval.

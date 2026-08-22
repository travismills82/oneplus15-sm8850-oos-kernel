# Controlled-stack release-contract architecture 2

## Decision

The controlled stack now has three independent identities:

1. **Kernel contract identity** controls `UTS_RELEASE`, `kernel.release`, and
   module vermagic.  It is derived from the selected Kleaf `kernel_build`
   inputs and guarded by the generated kernel contracts.
2. **Module source identity** records each external module family's source
   independently.  A DDK-only update changes this identity without claiming a
   new kernel ABI generation.
3. **Signing identity** records the controlled-v1 certificate, stock trust
   certificate, trusted bundle, and signing generation.

The Bluetooth `.046` import is external-module-only.  It does not change the
qualified kernel contract and therefore does not create a new boot generation.

## Qualified kernel contract

| Field | Qualified value | Bluetooth `.046` clean build |
| --- | --- | --- |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` | exact match |
| Release-stamp source | `6744a3f6bcf4226e37fa04475bd83a84fd3c3701` | reused only after guard |
| Kernel action source | `09a1b3e0167351cd7ae21d66cd4be8a300534e9f` | unchanged inputs |
| Common `.config` | `b53d48b303059adb49a8dbe457145a4b7523a77fae621ea8d9e7b0b727e1615b` | exact match |
| Canoe `.config` | `b5d038e4e03dde1664b036b2d7f2d6319d8089ce5f6987010a968fcefd1c7925` | exact match |
| `Module.symvers` | `de57709f3de38afb3e266481da09433687979ffb88ee607bda93ac4732dd7e0b` | exact match |
| `System.map` | `4f8ef4feb7c71abc18def8db341701e05b1ff7c0e8c8b50de7da48ef100f011a` | exact match |
| `modules.builtin` | `f0caaac04ccb3a60cf2c99036ac755e1df535cbf50004621286ea1aa90890760` | exact match |
| ABI | qualified baseline | PASS, empty report |
| KMI | qualified baseline | PASS |

The `KernelBuild` action query contained 101,648 inputs.  Its roots were
limited to `common/`, `build/`, `external/`, `prebuilts/`, and generated
`bazel-out/` inputs.  No path under
`vendor/qcom/opensource/bt-kernel/` was an input to the action producing
`vmlinux`, `Image`, `Module.symvers`, or `kernel.release`.

The clean rebuilt `Image` differs from the retained qualified `Image` in 20
bytes of GNU build-ID payload only.  The clean rebuilt `vmlinux` differs in the
same 20-byte build ID and one 16-byte non-allocating DWARF line-table source
checksum.  The verifier names those exact ranges and rejects any difference
outside them.  Executable and allocated data differences are zero.

## Kleaf/DDK inheritance

`tools/build-controlled-v1-external-ddk.sh` creates the reviewed release stamp
only after the kernel input scope is clean, builds the selected
`//common:kernel_aarch64`, and runs
`tools/verify-controlled-v1-kernel-contract.py`.  The three Bluetooth DDK
targets then consume that `kernel_build` target's:

- `include/config/kernel.release`;
- `Module.symvers`;
- generated headers;
- module build outputs.

The DDK source repository HEAD is not used as a second vermagic identity.
Changing a guarded kernel input, generated config, export contract, signing
certificate, or any functional Image/vmlinux byte fails before a DDK build is
accepted.

## Independent module and signing identities

| Family | Generation | Source identity |
| --- | --- | --- |
| system-DLKM | controlled-v1 WLAN053 | `6744a3f6bcf4226e37fa04475bd83a84fd3c3701` |
| WLAN | `AU_TECHPACK_WLAN.LA.2.0.R3.00.00.00.000.053` | `6744a3f6bcf4226e37fa04475bd83a84fd3c3701` |
| Bluetooth vendor | `AU_TECHPACK_BTFM.LA.2.0.R1.00.00.00.000.046` | `8906fd47be43616ee8ed532ae571ecbe30dced49` |
| Cellular | stock OOS 16.0.9.400(EX01) | stock; 27 exact binaries |
| NFC vendor | stock OOS 16.0.9.400(EX01) | stock |

Signing generation is `controlled-v1`:

- project certificate DER SHA-256:
  `04093d5e0b816927dc1e4fcdbd4ec754df6e69960918aa2eaa69bea6878f6faf`;
- retained stock certificate DER SHA-256:
  `f28dbcc60085b21a3cff1342482897fa640b468847473147834f26c4feb2df43`;
- trusted bundle SHA-256:
  `c1c17586a8bbd564a39b297d39a267b02ab4a266c0deaf81d536d026e68188a7`.

No private signing material is tracked or packaged.

## Bluetooth module contract

All three `.046` modules use exact vermagic
`6.12.23-android16-5-o-g6744a3f6bcf4-4k SMP preempt mod_unload modversions aarch64`
and signer `OnePlus 15 Controlled OOS Module Signing v1`.

`btpower` and `btfm_slim_codec` preserve their complete import/export CRC
sets.  `bt_fm_swr` preserves its exports and adds one import:
`swr_read` with CRC `0x5e6b9976`.  The retained `swr_dlkm` provider exports the
matching CRC.  The exception is explicit; any other import-contract change is
fatal.

The complete 436-module candidate graph, including the flattened controlled
system-DLKM and all 479 stock vendor-boot modules, reports:

- unresolved imports: 0;
- CRC mismatches: 0;
- protected-export failures: 0;
- signature failures: 0;
- unexpected module changes: 0;
- exact-stock cellular hash failures: 0.

## Payload contract

The static candidate is named `controlled-v1-wlan053-bt046`:

| Payload | SHA-256 | Delta |
| --- | --- | --- |
| `boot.img` | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` | exact qualified WLAN053 image |
| `system_dlkm.img` | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` | exact qualified WLAN053 image |
| `vendor_dlkm.img` | `50943999a1b5c006d64b7397edeb1debff343fc8d5602c930820f820968f60b2` | only three Bluetooth module payloads changed |

The candidate retains the 46-entry system-DLKM load contract with `wwan.ko`
at entry 21, all qualified WLAN053 module hashes, and all 27 exact-stock
cellular hashes.  Its ext4 filesystem passes read-only `e2fsck`; its regenerated
partition-local AVB hashtree/FEC/footer passes `avbtool verify_image`.

This candidate is **statically validated only**.  It has not been flashed or
physically qualified.


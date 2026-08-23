# Camera `.073` selective RER static validation

## Result

`PASS — READY FOR PHYSICAL TEST`

The complete `.073` Camera generation is rejected because its ISP, UAPI,
SMMU, synchronization, sensor, proprietary userspace, firmware, and foreign
device-tree closure is too broad for CPH2747. The candidate imports only the
RER command snapshot from `.073` into the authoritative `.061` Canoe camera
source.

The selected change copies the fixed-size RER flash command from
userspace-shared memory into private kernel memory, detects a concurrent count
change, validates the private count, and uses only that snapshot. This is
bounded race/bounds hardening in `cam_flash_pmic_pkt_parser()`.

## Kernel and build contract

- release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- kernel-contract guard: PASS
- `.config`: IDENTICAL
- `Module.symvers`: IDENTICAL
- Image/vmlinux functional contract: IDENTICAL
- dist: PASS
- ABI: PASS, empty report
- KMI: PASS
- controlled-v1 signing identity: unchanged

The build explicitly selects the Canoe Camera project configuration. Without
that selector, Kleaf correctly exposed a contract failure: the module omitted
qualified Synx/MMRM/SMMU paths and changed its imports. That output was
rejected and never packaged. With the Canoe selector, the candidate retains
all 421 imports, 158 exports, and their CRCs exactly.

Machine-code comparison shows the candidate parser has one additional
`cam_common_mem_kdup()` call and one corresponding
`cam_common_mem_free()` call versus the qualified camera module, matching the
reviewed private RER snapshot and cleanup path.

## Minimum delivery closure

| Module | Action | Reason |
|---|---|---|
| `camera.ko` | source replacement | selected `.073` RER snapshot hardening |
| `camera_extension.ko` | stock-source controlled re-sign | normal-boot consumer of protected Camera exports |

The Camera extension retains its exact allocated ELF sections, imports,
exports, CRCs, modinfo, and runtime payload. Only non-allocating symbols are
stripped before signing so the fixed filesystem geometry remains valid.

The stock vendor_boot contains a second `camera_extension.ko` with 49 imports
from the newly controlled provider. It is absent from vendor_boot
`modules.load` and is therefore a reviewed dormant boundary. The candidate
does not broaden recovery camera support.

Static module result:

- vendor modules: 436
- intended replacements: 2
- unexpected replacements: 0
- unresolved imports: 0
- CRC mismatches: 0
- protected-export failures: 0
- signature failures: 0
- system `modules.load`: 46 entries
- `wwan.ko`: entry 21
- ext4: PASS
- partition-local AVB/footer/hashtree: PASS

## Candidate

- image: `out/camera-rer-073-candidate/vendor_dlkm.img`
- SHA-256: `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`
- boot: unchanged
- system_dlkm: unchanged
- vendor_boot: unchanged
- VBMeta: unchanged
- physical flash: NOT PERFORMED

The physical test must change only vendor_dlkm after a verified hardened-TWRP
backup, dry run, and exact write read-back. It must exercise all CPH2747
cameras, still capture, flash, video, camera switching, focus/zoom, open/close
stress, recording audio, suspend/resume, and reboots. The hostile RER race must
not be injected; if the command path is not naturally observed, report normal
compatibility separately from hardening-path coverage.


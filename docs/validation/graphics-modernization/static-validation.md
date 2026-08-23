# Graphics `.057` selective secure-guard candidate

## Decision

The complete `.057` Graphics drop is not accepted for Canoe.  It changes the
active Gen8.2 GMU/HFI, power, reclaim, device-tree and firmware-facing
contracts.  The candidate imports only the bounded
`kgsl_free_secure_page()` ownership change from `.057`.

The change checks the secure-world unlock result before returning the secure
guard page to the allocator.  On failure the page remains allocated and an
error is logged.  This is classified as memory-ownership hardening; normal
runtime qualification cannot prove the error branch unless it occurs
naturally.

## Source and build contract

- baseline synchronization: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- comparison synchronization: `d447f713d6403f707a2910383495f4ada98cfa4d`
- selected source object: `ac4999291959c8a9f49994e8a6e9f378dce64eb7`
- kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- kernel-contract guard: PASS
- `.config`: IDENTICAL
- `Module.symvers`: IDENTICAL
- Image/vmlinux functional contract: IDENTICAL
- dist: PASS
- ABI: PASS, empty report
- KMI: PASS

The generated `msm_kgsl.ko` machine code contains a conditional branch on the
secure unlock return value, reaches `__free_pages` only through the success
path, and reaches `_printk` through the failure path.  Its 531 imports and six
exports, including all CRCs, match the qualified baseline exactly.

## Minimum protected-export closure

| Module | Action | Reason |
|---|---|---|
| `msm_kgsl.ko` | source replacement | selected `.057` function-body change |
| `oplus_bsp_geas_system.ko` | stock-source controlled re-sign | normal-boot consumer of protected `geas_update_gpu_params` |

The candidate keeps 434 other vendor modules byte-identical.  GEAS retains
the exact allocated ELF sections, imports, exports, CRCs and modinfo; only
non-allocating symbols are stripped to fit the fixed filesystem geometry and
the result is signed with controlled-v1.

One retained vendor_boot GEAS copy imports the newly controlled protected
provider.  That copy is not requested by the vendor_boot normal-boot load
policy and is recorded as a reviewed dormant boundary.  The physical test
must use the normal Android path and must not broaden recovery support.

Static module results:

- vendor modules: 436
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

- image: `out/graphics-secure-guard-057-candidate/vendor_dlkm.img`
- SHA-256: `a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`
- boot: unchanged from the Audio-qualified baseline
- system_dlkm: unchanged from the Audio-qualified baseline
- vendor_boot: unchanged
- VBMeta: unchanged
- physical flash: NOT PERFORMED

## Required physical qualification

Flash only `vendor_dlkm` after a verified TWRP backup and dry run.  Require an
exact post-write read-back hash before boot.  Exercise UI rendering, all
supported refresh rates, OpenGL ES, Vulkan, hardware composition, video,
gaming/GPU stress, frequency and thermal transitions, screen off/on, deep
idle, three clean reboots and camera preview.  Preserve WLAN, cellular,
Bluetooth, NFC and Audio qualification.  Scan full logs for KGSL, GMU, Adreno,
IOMMU/SMMU faults, GPU hangs/resets, memory faults, module-contract failures,
Oops, BUG, panic and call traces.

The secure-unlock failure branch is not to be fault-injected.  If it is not
naturally observed, report normal-runtime compatibility separately from the
unexercised hardening path.

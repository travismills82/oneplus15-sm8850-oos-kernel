# ColorOS 17 multimedia driver modernization

## Scope and evidence boundary

This is an experimental matched-module source/build investigation for OnePlus 15
ColorOS 17.0.0.102. It does not change the released boot-only artifact, stock
partitions, firmware, device tree, calibration, or the qualified common kernel.
Source fixes are not deployed fixes: the phone continues to use its stock
modules until a separately authorized, validated package is installed.

The unchanged starting revision for these controls is
`0c4d5b23ab7ca52462fd40cb1c9ecb02438a0fe0`. The common-kernel tree remains
`818ee1b0ff65d6d475afeeba55bf4099cfcbfc22`. Normal Canoe DDK target configuration
and generated makefiles are used with the frozen qualified common build outputs;
no generic substitute configuration or synthetic symbol-version table is used.

Private evidence root:
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/multimedia/`.
It contains build inputs/results, logs and bounded upstream tag advertisements,
not release assets. No physical runtime result is claimed by this document.

## Authoritative reference refresh

The following official CodeLinaro tag advertisements were refreshed on
2026-10-30. These are reference endpoints, not proven exact source provenance for
the shipping proprietary module binaries. No complete endpoint is imported.

| Subsystem | Official repository | Latest inspected same-family release | Peeled commit |
|---|---|---|---|
| GPU | `clo/la/platform/vendor/qcom/opensource/graphics-kernel` | `GRAPHICS.LA.16.0.r1-10300-kaanapali.0` | `adf5712becd72bce1c6b426b41ed82545dd923bb` |
| Camera | `clo/la/platform/vendor/opensource/camera-kernel` | `CAMERA.LA.6.0.r1-15300-kaanapali.0` | `a895695349f3ca188289e8674f57aac82ac1016c` |
| Video | `clo/la/platform/vendor/opensource/video-driver` | `VIDEO_HANDSET.LA.1.0.r1-12600-kaanapali.0` | `359c5e3e7f7fad9c62e507a70334e08d73fc200a` |
| Display | `clo/la/platform/vendor/opensource/display-drivers` | `DISPLAY.LA.6.0.r1-17700-kaanapali.0` | `8ca3facf30676af748831e2c6eb8bc02141df6aa` |
| Multimedia synchronization | `clo/la/platform/vendor/opensource/mm-drivers` | `DISPLAY.LA.6.0.r1-17700-kaanapali.0` | `81b311f44c805c826de609186a12cb9660172949` |

All URLs have prefix `https://git.codelinaro.org/` and suffix `.git`; the
partition-specific shipping inventory remains the authority for actual runtime
provider identity. Tag advertisements are under `multimedia/refs/`.

## First bounded implementation group

Each row is an independent upstream patch; downstream Git SHAs are recorded
after the owner commits them. Relative patch IDs are calculated after removing
the repository's vendor-path prefix and match the authoritative patch content.

| Upstream commit | Change | Relative stable patch ID | Source review |
|---|---|---|---|
| `1d15bd800fe5dad77ba5a4a4571e1478bae3a52e` | KGSL split-VMA fault offset and memstore offset normalization | `c4475240f6e5458e1dc8cb0e4f46a778b2460419` | No header/layout/config change; existing fault bounds and page locking retained |
| `abb98ee3b27b9df161a490a15ccdd3c22ab712c0` | JPEG release obtains manager lock before context access | `1677f7cc2067af27e76bd022a2de54ed821c601d` | Manager-before-context ordering agrees with acquire; early inactive-context exit now owns the lock it unlocks |
| `afacc37ba1c8ff3eeb9cdbe02be4fe87a5b52e32` | IFE composite-group capacity check before indexed writes | `a595518317d117e86f32f6763efcec3460aceb8c` | Existing error unwind retained; no public/private layout change |
| `047de0202db8b28c0cfa99ed11702691b86e883e` | ICP checks room for each remaining dump block | `3420e0ee8a153b148dd698ffdfbba1364149f8ad` | Every new failure uses existing CPU-buffer release label; no firmware protocol change |
| `d58c17180c41de236c408313b1a96e348557f7b5` | OPE dump bounds cover the actual copied descriptor size | `e9f3ea739956f97406f63dafa76db4cf1367f7c0` | Existing offset and size-versus-length checks retained |
| `668f72743a751668a259b1dc488dfa2f6e6b00ff` | ISP dual-config validates output port range and map sentinel | `2ef1e46fb27b708498f1cd73fdc7e02baaa270f7` | Validation precedes mapped-resource indexing; no exported prototype change |
| `11a7a73b3b73242ec97343edd41de422015613ff` | IFE/SFE FCG validates computed data size against real blob length | `905f9e2220d10de315cc79af530fdbb4724331a6` | Only a static helper gains an argument; both call sites pass the separately validated length after nonzero/hardware-cap count checks |
| `ce2c54db852c0171cc6256cc78be42a1cdc3b1db` | Clear cached request-map pointers on cleanup and reject invalid reuse | `d7d864550846ff73b8259dbb8fbdf940877eaa06` | Request storage is initialized before allocation; configuration/release entry points hold `ctx_mutex`; no request structure change |
| `408f334b69bb1cd08b973dace1908e0b0a98b430` | Camera DMA-fence recycled-row ownership validation and pre-put logging | `54bd2f2d4449a6a7e993c7e9ecdb648d79556382` | Internal, nonexported helper and every in-module caller migrate together; no data-layout/UAPI change; prerequisite for 15300 last-put fix |
| `33aa6410967d8308b97678ba6488f1ff915a43c4` | Camera DMA-fence last-put invalidates the row; invalidated allocated rows can be reclaimed | `25b7328c2fe3640363fa272d4b5db2c266db1bad` | Retains current release callback; row lock protects invalidation; paired with the separately documented local invalid-row cleanup |
| `cfed001431a2e623154fde329a31e9fa2a534b31` | Video SSR allowlist validation before mutating pending SSR type | `7f9b4983f49f5fea0ffc14dce40bc0e37d4bf00b` | One `.c` function-local variable; does not claim to fix every SSR lifetime race |

These fixes require rebuilding the actual multimedia provider modules. They do
not make the corresponding untouched stock module binaries fixed. Compilation,
export/import CRCs, private layouts, KCFI identities, provider precedence,
signatures, and physical/HAL regression tests remain independent gates.

The OPE command-buffer file is **not selected by the present Canoe DDK source
list** and is absent from the complete control compile log. Its source-level
backport must not be counted as an active compiled Canoe protection. No OPE
configuration is enabled by this task. The current manager's separate request
copy had the same length-versus-size defect. A separate local follow-up now
checks its actual copied size and preserves `-EINVAL` on failure instead of
returning success with a cleared output pointer. It is not silently included in
the original one-line upstream patch and remains dormant/uncompiled for Canoe.

The last-put row invalidation also needed two local coherence corrections:
monitoring an invalidated row reports reference count zero without dereferencing
a now-NULL fence, while retaining history/error diagnostics; an invalid-row
failed put releases the caller-owned `expected_fence` after dropping the row
spinlock. The original import error path owns that reference, acquired by the
earlier lookup, and does not drop it elsewhere. No lock is weakened and no live
row/foreign-fence reference is fabricated.

### Isolated downstream commits

| Upstream fix / local follow-up | Downstream commit |
|---|---|
| KGSL `1d15bd8` | `6285ba23b5c97cc019699ab5cad0531cad8dc18a` |
| JPEG `abb98ee` | `93c8986ac3f14bd750a14a5f61badcb834291af9` |
| IFE group `afacc37` | `ba59514314ee940bc060ac66c844d018703c40b1` |
| ICP dump `047de02` | `409e9b3d9196cc134f157269b8e53a991a3adc83` |
| Dormant OPE dump `d58c171` | `272c2320d56625c88d10dd43aadc8babd8d0da10` |
| ISP dual config `668f727` | `640d1bff2818c81ad401594d476c4b56c6652ed3` |
| FCG blob `11a7a73` | `3a04135a392d5d10d5deea35bc9f6e54040857ec` |
| Context map lifetime `ce2c54d` | `2ff82d357f07cdafe8dbfd7910d470d184b45003` |
| Fence prerequisite `408f334` | `8c8aadeb9662f65ef9e49b1814df594dde220c26` |
| Fence last put `33aa641` | `e3e7b8821eabb79e1913210e83dcabaf60b81263` |
| Video SSR validation `cfed001` | `7f2d88b48379f62f6a151fa376c797e65419b1f1` |
| Fence invalid-row/monitor coherence | `fbdf1d7c671b8ba96b2c1a1f7bc316a2a0f3325e` |
| Dormant OPE request copy bounds/error | `e41f8fbecf6d7bf233527ba26eff278fa0c5a15f` |
| Context diagnostic whitespace | `92ff219b2b6df5339efe0b2c8cae20a65e26ab09` |

## Control-build status

| Target | Control evidence | Observed result |
|---|---|---|
| `//vendor/qcom/opensource/graphics-kernel:canoe_perf_msm_kgsl` | `gpu-control-01/` | Unchanged control build PASS with real provider SDKs; shipping-interface audit FAIL below. |
| `//vendor/qcom/opensource/camera-kernel:canoe_perf_camera` | `camera-control-01/` | Build PASS on frozen-source `sdk01` retry with all 33 genuine cached dependency providers; module SHA256 `760dc04694ba9b106d6327b891832fa60c872083c8490c6fbb6e334d3e5922e7`. Shipping-interface audit FAIL, as detailed below. |
| `//vendor/qcom/opensource/video-driver:canoe_perf_msm_video` | `video-control-01/` | Unchanged control build PASS with real provider SDKs; shipping-interface audit FAIL below. |
| `//vendor/qcom/opensource/display-drivers:canoe_perf_msm_drm` | `display-control-01/`, then `display-control-02/attempt-sdk01.json` | Initial missing-source failure preserved. After exact manifest restoration, fresh unchanged control02 builds PASS with 31 genuine provider SDK tables; module SHA256 `279e03dbd282d0fdd686dede77e3eff56697fabcf95c4da453e368d799bfd913`. Shipping-interface qualification is assessed separately below. |
| `//vendor/qcom/opensource/mm-drivers/sync_fence:canoe_perf_sync_fence` | `sync-fence-control-01/` | Unchanged isolated module build PASS; SHA256 `9867adc2809a9b06348ab2e7df465aa8673b94cffcd7c60a37723760bf6a8e5e`. This is not a security-fixed or packaged module. |

The frozen source snapshots permit unchanged controls to be retried with
legitimate generated inputs without accidentally testing the patched sources as
the control. No missing-symbol warning is suppressed.

### Exact manifest display-source restoration

The missing OPlus directory was **not unavailable public source**. The exact
modules commit already named by `oneplus_15.xml`,
`5ab2a689ff87d7d28c511f1762cf41c1b90d965a`, contains 57 regular source files under
`vendor/qcom/opensource/display-drivers/oplus`. They are not generated symlinks
or copies from another vendor path. The initial custom import `234073f08cd43`
omitted all 57. The nested `.gitignore` explicitly ignores `oplus`, so a normal
unforced import would omit them; that is consistent with the observed loss,
rather than proof of an unavailable source archive.

The manifest commit's author is `pswbuild <pswbuild@oppo.com>`, dated 2026-07-23,
and identifies OnePlus 16.0.9.400 source synchronization. This provenance is **not
a claim that these sources equal shipping ColorOS 17.0.0.102 binaries**.

The separately isolated local restoration commit `4237c3765dca9` adds only the
57 exact original blobs: 1,123,071 bytes, all mode `100755`. Directory tree ID:
`2b988ef69185701366472f18339576ac4b3ecf98`. After restoration, the **whole display
source tree** equals the pinned manifest tree
`cec62246e230b30a87456ad49df404feb205dafd`. No display behavior patch was added.

Procedure: enumerate the pinned Git tree, refuse every existing destination,
obtain only its exact blobs (local objects where available, otherwise official
OnePlusOSS raw URLs pinned to the same full commit), verify Git object identity
before editing, create files through `apply_patch`, restore only the exact
original line-ending formatting, and verify read-back SHA256, Git blob ID and
mode for all 57 files. Existing files overwritten: zero. Eight originals have
mixed/CRLF or missing terminal-newline formatting, preserved byte-for-byte.
Consequently the restoration's `git diff --check` reports inherited whitespace;
it is not described as whitespace-clean or silently normalized.

The formerly missing `oplus/common/trackpoint/oplus_trackpoint_report.h` has
SHA256 `0d314290513c90d57814bab24e3beb4ef7f2e4050fee3f6ac32fc88e86a8e9ad`.
Complete 57-row proof and source URLs:
`multimedia/display-source-restoration.json` in private evidence. A fresh control
was necessary: retries of the old frozen missing-source control must not import
these new files silently. The original failure is preserved, and control02
captures all restored source inputs independently.

The normal dependency-SDK closure is generated without changing production
providers. Four additional unchanged controls compile successfully using the
normal prepared target configuration and the exact frozen common outputs:

| Control target | Module SHA256 |
|---|---|
| `canoe_perf_msm_hfi_core` | `3b4bb79d654390d3b0855c9da2aadce0fde5ece5721d426c81c1b831080e10d7` |
| `canoe_perf_msm_ext_display` | `010aed5925dc6cfb28252e1f6a641d276c3c88f31c391efba3cff9c919de04e7` |
| `oplus_bsp_tp_notify` | `7fd0b8f8d27e3dd3f011046b3abd7b185cee0b377de83b2db3f61c6511e02e86` |
| `canoe_perf_hdcp_qseecom_dlkm` | `4b2930a51cb892080136bbaa0f646846c73fe4bd791bb364ac6d48f0253fc7ed` |

Their complete real `Module.symvers` outputs are build SDK inputs, **not proposed
runtime module replacements**. HFI, external-display, touch and HDCP behavior are
unchanged, and passing compilation does not prove their stock/private ABI or
hardware compatibility.

Read-only audits of those unchanged controls confirm why SDK provenance and
runtime replacement eligibility remain separate. Touch-notify matches all five
stock exports, every version/import record, all three function bodies and all
24 retained-consumer edges. External-display matches all four exports and every
version/import record, with six matching retained edges; one function body
differs and still needs review before any replacement. HFI has clean imports
but differs on `hfi_core_map_sg_table`, `hfi_core_notify_rsp_timeout`,
`hfi_core_open_session` and `hfi_core_unmap_iova`. Eight retained edges (four
distinct interfaces, with normal/dormant stock copies) fail, and the mapped
KCFI word of `hfi_core_dgb_client_cb` differs. These are unmodified-public-source
versus stock differences, not newly introduced security-fix regressions. None of
these SDK controls is added to the production replacement population.

The separately audited unchanged HDCP control matches all 16 stock exports,
22 version records and 21 undefined ELF imports. Its 72 function bodies and
68 mapped KCFI words are byte-identical to stock; all 28 retained `msm_drm`
import edges pass. Its two required SMCInvoke SDK exports also match stock
naturally. Neither HDCP nor SMCInvoke is replaced in the production population.

Display's completed `sdk01` retry records source HEAD
`566ab26631116bdb8d9cb8772a8d3cbf5f5f44fc`, after the exact restoration; its display
source tree is unchanged by the later unrelated WLAN commits. The retry did not
reload source from the live worktree, and both source-input and source-closure
integrity checks pass. It uses 24 genuine cached producer tables plus the seven
unchanged SDK controls for hardware fences, sync fences, Synx, HFI, external
display, touch-notify and HDCP. Output `Module.symvers` SHA256:
`c64d3d6ed82377a8aa90b7e6c4c52d6a746ac78839517f4bbc47b2fe9822c1fc`.

### Restored unchanged display control: actual shipping compatibility blocker

`display-control-interface.json` compares the unchanged restored public-source
module against exact stock `msm_drm.ko`, SHA256
`e01da769c0a45c94bcdf9c93689ded8f7ba05f943a241f01bfc99c066475b3f1`.
It reports 400 stock versus 401 rebuilt
exports, 28 export differences, 27 version-record differences, 17 undefined-import
differences, and 17 differing mapped KCFI words. There are 2,992 byte-identical
function bodies, but also 1,837 differing common functions, 115 missing functions
and 63 new ones. These differences are not attributed to the restoration: its
source bytes exactly match the manifest-pinned display tree.

Ten actual imports have CRCs incompatible with the retained stock providers:

| Actual import | Rebuilt display expected CRC | Shipping provider CRC |
|---|---|---|
| `hfi_core_open_session` | `0xec495e5e` | `0x7a12e93a` |
| `socinfo_get_part_count` | `0x4cf57500` | `0x94cb45f2` |
| `socinfo_get_part_info` | `0x8d1a6de2` | `0xfff0ca6b` |
| `socinfo_get_subpart_info` | `0x649b5cf3` | `0x8d422d74` |
| `synx_create` | `0x185b8fe2` | `0x748cdacc` |
| `synx_import` | `0x7c670d90` | `0x10b058be` |
| `synx_initialize` | `0x3f5ed08e` | `0x7bd39412` |
| `synx_release` | `0xc88ea397` | `0x700700ee` |
| `synx_signal` | `0x9e8a6859` | `0xa15fdef5` |
| `synx_uninitialize` | `0x72216f83` | `0x1403f625` |

`system_percpu_wq` resolves with the expected CRC, but is absent from the actual
compiled unsigned-module import allowlist. The ten unresolved CRC edges also
fail the import-policy gate; eleven policy failures are therefore reported, not
eleven additional independent mismatches. Protected-export collisions: zero.
No retained shipping module imports a display export in the audited complete
module graph; this does **not** prove absence of HAL, callback, private-structure,
user-ABI or firmware/calibration requirements.

Mapped KCFI changes include `callback_function_hfi`, `dp_hdcp2p2_register`,
`dsi_display_get_modes_helper`, ten `hfi_*` helpers, `oplus_apuir_set_cmd`,
`sde_encoder_phys_vid_control_esync_vsync_irq` and two
`sde_hw_sspp_setup_format*` functions. Exact names, code differences, providers,
CRCs and policy outcomes are preserved in the private JSON. They are not hidden
with casts, source-layout guesses or forced CRCs. Display remains **quarantined**;
no new display behavior patch or display replacement enters the production
module population.

### Unchanged camera control: actual shipping compatibility blocker

`camera-control-interface.json` compares the compiled **unchanged** public-source
control against exact ColorOS `.102` stock `camera.ko`, SHA256
`d23949ea1635115241b1c4ad1ccdfd80e925837f244086773907e7a1f2972193`.
It identifies 28 export differences, 29 version-record differences, 27 undefined
ELF import differences, and two differing KCFI prefixes (`cam_soc_util_get_mem_base`
and `cam_sync_util_cb_dispatch`). None is attributed to our security patches.

Two actual imports cannot resolve against retained shipping providers:

| Import | Unchanged control CRC | Stock `vendor_boot:socinfo` CRC |
|---|---|---|
| `socinfo_get_part_count` | `0x4cf57500` | `0x94cb45f2` |
| `socinfo_get_subpart_info` | `0x649b5cf3` | `0x8d422d74` |

Current declarations use `enum subset_part_type`; its exact shipping definition
has not been established. Restoring a CRC without that enum/type provenance is
not authorized. Shipping `socinfo.ko` has no BTF/DWARF layout evidence in the
inspected section inventory.

The retained `camera_extension` consumer has 98 total import edges to camera
across the normal vendor copy and dormant vendor-boot copy. Forty-two fail against
the unchanged replacement (21 distinct exports, listed below). The report's
edge records retain their original all-stock `result=MATCH`; that old field must
not be misread as candidate compatibility. Candidate validation independently
compares the candidate's true exports/CRCs, not a substituted stock provider.

| Export required by `camera_extension` | Stock expected CRC | Unchanged control CRC |
|---|---|---|
| `cam_actuator_power_down` | `0x0381bb1e` | `0xda237800` |
| `cam_actuator_power_up` | `0x0381bb1e` | `0xda237800` |
| `cam_handle_mem_ptr` | `0x282316d1` | `0x19709ad4` |
| `cam_ois_driver_cmd` | `0xc0335ee3` | `0x1422e4be` |
| `cam_ois_pkt_parse` | `0xc0335ee3` | `0x1422e4be` |
| `cam_ois_power_down` | `0x7ea02310` | `0x9306f3f2` |
| `cam_ois_shutdown` | `0xa87b1802` | `0x2844d3f4` |
| `cam_sensor_apply_settings` | `0x63a090a9` | `0xc1e13cd8` |
| `cam_sensor_core_power_up` | `0xbdbc2dd3` | `0x8c355d49` |
| `cam_sensor_driver_cmd` | `0x2a0a8430` | `0x0bbc8ec6` |
| `cam_sensor_get_dpc_data` | `0x41f64506` | `0xccb90fe8` |
| `cam_sensor_match_id` | `0x41f64506` | `0xccb90fe8` |
| `cam_sensor_power_down` | `0x41f64506` | `0xccb90fe8` |
| `cam_sensor_power_up` | `0x41f64506` | `0xccb90fe8` |
| `cam_sensor_read_qsc` | `0x41f64506` | `0xccb90fe8` |
| `cam_sensor_util_power_down` | `0x260bbbe3` | `0xf8af5e1e` |
| `cam_soc_util_get_dt_properties` | `0x2ec8e2cb` | `0x12b17d53` |
| `cam_soc_util_register_monitor_track_regulator` | `0x17fff3b5` | MISSING |
| `msm_camera_fill_vreg_params` | `0xeb8a78bd` | `0x112933b8` |
| `oplus_cam_actuator_construct_default_power_setting` | `0xad8fdb50` | `0xf375792e` |
| `tracing_mark_write` | `0xbac6a29a` | MISSING |

`vendor/oplus/kernel/camera/camera_extension` source is present, so a matched
rebuild may resolve some private interfaces, but would expand the replacement
closure and is not proof of proprietary HAL/calibration parity. A replacement
camera package is **not approved** by the successful compile. Preserve this
independent baseline mismatch, continue source hardening, and measure candidate
drift relative to the unchanged control without disguising the shipping failure.

The minimal `//vendor/oplus/kernel/camera:canoe_perf_camera_extension` source
closure was built unchanged with the genuine camera control SDK. Build: PASS.
Its 21 imports that disagree with stock camera naturally match all 21 rebuilt
camera exports, with no CRC editing. That demonstrates a coherent rebuildable
pair, not approval to ship either module. It does not fix camera's two retained
socinfo-provider disagreements or establish proprietary HAL/OIS/calibration
parity. Neither member is added to the deployable package.

### Unchanged GPU and video controls: shipping blockers

GPU control has 11 failed provider imports and 12 unsigned-import policy
failures, including the following exact natural CRC disagreements:

| GPU import | Compiled control CRC | Shipping provider CRC |
|---|---|---|
| `qcom_iommu_set_secure_vmid` | `0x9aa7e17e` | `0x913ad69d` |
| `register_mtrack_debugger` | `0x561885cb` | `0x20577724` |
| `register_mtrack_procfs` | `0xa18d2361` | `0xba9a9d4d` |
| `unregister_mtrack_debugger` | `0x00c01888` | `0xf78ca29e` |
| `unregister_mtrack_procfs` | `0x8a1380b4` | `0x5bf9e57a` |
| `synx_create` | `0x185b8fe2` | `0x748cdacc` |
| `synx_import` | `0x7c670d90` | `0x10b058be` |
| `synx_initialize` | `0x3f5ed08e` | `0x7bd39412` |
| `synx_release` | `0xc88ea397` | `0x700700ee` |
| `synx_signal` | `0x9e8a6859` | `0xa15fdef5` |
| `synx_uninitialize` | `0x72216f83` | `0x1403f625` |

The extra unsigned policy failure is `system_percpu_wq` (common-header alias
evolution), not an extra CRC disagreement. The sole GPU export difference is
`get_gpu_bw_data`, causing two failed `oplus_bsp_geas_data` edges (normal vendor
and dormant vendor-boot copies). The other two retained GPU edges from
`oplus_bsp_geas_system` match. None is introduced by the offset fix.

Video control exports match stock, but has eight failed imports: both socinfo
functions above plus the six listed `synx_*` APIs. Its additional unsigned policy
failure is `system_percpu_wq`. Retained video-consumer export failures: zero.
These are pre-existing control results, not introduced by SSR validation.

Unchanged `msm_hw_fence.ko`, `ipclite.ko`, `synx-driver.ko` and `msm-mmrm.ko` were
built only to obtain legitimate SDK tables where the original frozen cache did
not contain them. They are **not runtime replacement candidates**. In particular,
the rebuilt synx control itself differs from stock (17 exports, 39 retained
consumer-edge failures, two failed provider imports); choosing its SDK to link a
module does not make that module compatible with stock synx. The production
oracle continues to use actual retained shipping providers.

## Patched control-to-candidate results

All three selected candidate module builds pass normal compilation/modpost using
the legitimate producer tables. No modpost, signature, CRC, CFI or KMI check was
disabled. These are individual DDK module builds against the qualified common
SDK, **not a new full production distribution or physical qualification**.

| Provider | Patched `.ko` SHA256 | Export / MODVERSIONS / ELF-import differences vs control | KCFI prefix changes | Expanded DWARF layouts |
|---|---|---|---|---|
| GPU `msm_kgsl` | `a9d4f8ebf721bc3b0c8ffe9cac661d56a9118e9c87480b6e5a3f557f28795843` | `0 / 0 / 0` | 0 | 3 checked, 0 differences |
| Camera | `51e4af095c1c7f45cff94e58cb26d1a71724fd58203a0a75282be86a4434c42f` | `0 / 0 / 0` | 1 internal helper, explained below | 6 checked, 0 differences |
| Video `msm_video` | `a73649a092b1adf5c8d386d997e157ddac53c239c408db021ce00f7fec1a9a2e` | `0 / 0 / 0` | 0 | 3 checked, 0 differences |

GPU layouts checked: `kgsl_process_private`, `kgsl_memdesc`, `kgsl_device`.
Camera layouts: `cam_context`, `cam_ctx_request`, `cam_ife_hw_mgr_ctx`,
`cam_jpeg_hw_mgr`, `cam_dma_fence_row`, `cam_sensor_ctrl_t`. Video layouts:
`msm_vidc_core`, `msm_vidc_ssr`, `msm_vidc_inst`. These are actual compiled
control/candidate DWARF comparisons; absent stock DWARF is not invented.

The one camera KCFI change is `cam_dma_fence_get_put_ref`: an internal,
nonexported helper deliberately migrated from two to three arguments together
with every caller. Full vendor source search finds only the three in-module
callers and its declaration/definition. Exact-string search across all stock
module binaries finds this name only in stock camera itself. It is not exposed
as a stock module export or supplied as an external callback. No incompatible
function-pointer cast or forged CFI identity is used.

GPU has exactly two changed function bodies: `kgsl_mmap` (inlined memstore reset)
and `kgsl_paged_vmfault`. Video has exactly one: `msm_vidc_trigger_ssr` (same byte
length, corrected assignment ordering). Camera has 166 changed function bodies;
all source locations correlate to the eight deliberately modified translation
units, with zero out-of-scope function locations. Source snapshots differ only
in the reviewed C/private-helper-header files and per-run generated flags paths.
Logging line constants and LTO/inlining can produce wider byte differences;
source-location correlation is **not** proof of instruction-level semantic
equivalence. LLVM emitted 613 `R_AARCH64_NONE` relocation diagnostic lines during
that correlation, preserved verbatim in private evidence; do not hide that host
tool limitation or treat correlation as a runtime test. No camera package is
approved on that basis.

Evidence: `*-candidate-vs-control.json`, `*-candidate-layouts.json`,
`camera-codegen-source-correlation.json` and its captured stderr in the private
multimedia evidence directory. **Shipping compatibility failures remain exactly
the pre-existing failures above; compilation and zero new public ABI drift do
not convert a failing stock boundary to PASS.**

## Explicitly held or excluded changes

| Upstream commit | Decision and concrete reason |
|---|---|
| `63ed21e8d3458b65d5777be01223dc210c76b666` | Hold: adds an `mm` pointer to private KGSL process state and changes allocation/destruction lifetime. Requires complete OEM/private consumer closure before a layout-changing integration. |
| `b0cca05eb46edbaefd97de700d3145e75d7642d2` | Do not apply verbatim: validation under `fence->lock` calls `dma_fence_get_status()`, which acquires that same lock in this common tree. It also unlocks before acquiring user fences and only relocks for replacement, allowing two binders to validate the same unbound array. A coherent prepare/revalidate/commit design is required; adding only the upstream lock is not sufficient proof. |
| `0ce79f17fe4b85e26611143fa387478904054cae` | Exact later `worker_ctx` source path is absent; do not introduce a new CRM worker architecture merely to apply its cleanup fix. |
| `7fc2d59a1f28556697aaf0367fe614e8c99e3470` | Hold: OPE DMI/state ownership and private header changes need a separate complete consumer/layout audit. |
| `8761385571e5cd187c556b202b8a18a2d58ea8b0` | Hold after complete cleanup/callback review: the proposed table mutex protects final free, but current context destruction happens first without that lock. Duplicate-bind cleanup also frees a pre-existing object it did not allocate. See the focused evidence below. |
| `de9c1ae1743fb45ea7b61ced0f8da5f11a98069c`, `bc5d4ffa329390049f3b72c2bb0fb941a1cec751` | Hold for state/power proof: current core initialization holds `core->lock` and enters INIT_WAIT, but `venus_hfi.c::__load_fw()` calls `__venus_power_on()` before `fw_load()/request_firmware()`. The upstream comment that hardware is not yet powered cannot establish suspend safety in this tree. No firmware blob is changed. |
| `a5d76cd06d628ae1b26f1131b95ac403ed122129` | Hold for complete teardown lifetime proof. A `core == g_core` test is not a reference: removal may follow the comparison. Here devm core cleanup was registered after the debugfs cleanup action, so reverse cleanup ordering can destroy the core mutex before debugfs removal. Do not label pointer comparison alone a complete UAF fix. |
| `260c3b898ff514104c96bdc50ad74770baa30f38` | Inactive for present Canoe configuration: normal target does not select `CONFIG_MSM_SDE_ROTATOR`/rotator sources. No unrelated driver feature is enabled to manufacture applicability. |

### ICP event-injection fix: why a lock-only import remains incomplete

The event callback is invoked from `cam_common_evt_inject_set()` in
`cam_utils/cam_common_util.c:1024`, through the registered callback at line 1105;
the setter is installed by `module_param_cb(cam_event_inject, ..., 0644)` at
line 1239. This is a sleepable process-context parameter operation, not an IRQ
callback, so taking a mutex is allowed.

The proposed callback ordering is `g_dev_lock` then `ctx->ctx_mutex`, followed by
the selected ICP event-injection implementation. No reverse acquisition of
`g_dev_lock` inside those inspected paths was found. That local lock-order result
does **not** establish full teardown lifetime correctness:

- `cam_icp_subdev_clean_up()` has exactly two callers: bind's `probe_fail` path
  and component unbind. The patch adds `g_dev_lock` only around its final
  table-entry free/clear.
- Unbind first obtains `g_icp_dev[device_idx]`, then calls
  `cam_icp_context_deinit()` for every context, then hardware/node/subdevice
  teardown and only finally cleanup. Context deinit calls
  `cam_context_deinit()`, which `memset()`s the context, including its mutex and
  state-machine pointer. That happens **before** the new final-free lock.
- Concurrent injection can therefore enter `cam_context_add_evt_inject()` while
  teardown clears the context. This helper checks `state_machine` before taking
  `ctx_mutex`, then uses it under that mutex. The proposed final-free mutex does
  not serialize this earlier destruction against the callback.
- The duplicate-bound check in `cam_icp_component_bind()` finds an already
  populated table entry and jumps to `probe_fail` before allocating anything.
  Cleanup then frees that pre-existing active entry. The official patch retains
  this separate ownership defect; adding a lock does not confer ownership.
- Simply clearing the table early is not an adequate repair: existing subdevice
  open/close routines retrieve their device through the same table and
  immediately dereference its `icp_lock`. They require a coherent lifetime/
  ownership design, not a new NULL window.

A complete backport must quiesce callbacks before context destruction and fix
bind-error ownership, while proving all open/close, cleanup and lock ordering.
No speculative quiescing flag, private layout change or partial lock-only patch
is added in this phase. `8761385` remains unapplied, and this camera source/runtime
gap is disclosed rather than claimed fixed.

## Focused host checks

`python3 -B tools/driver-updates/test_coloros17_multimedia_bounds.py` currently
passes 25 checks: ten range/arithmetic models and fifteen static source-ordering
contracts. They cover exact fits, insufficient later dump blocks, subtraction
underflow rejection, copied backing sizes, KGSL split-VMA offsets, FCG actual
length propagation, and critical guards before access. These are explicitly
**not execution of kernel code, locks, hardware, DMA, firmware or refcounts**.

`git diff --check` passes for the selected functional changes. Their aggregate checkpatch:
zero errors, zero warnings, 358 lines checked. The upstream `ce2c54db` diagnostic
spacing defect was corrected in its own downstream style commit after preserving
the exact original backport. Those counts exclude the later exact 57-file
manifest restoration and its independently documented inherited whitespace.

## Watchdog boundary

No multimedia patch is described as fixing the previous intentionally dormant
`adci_thread`, `zram_comp`, or `hfi_core_dbg_cl` waits. Video PM/SSR fixes concern
different paths; their applicability does not establish a cause for idle D-state
reports. Existing boot-only watchdog protections are untouched.

## Current disposition

GPU, camera and video source backports and controlled compilation are complete
for the selected bounded group. Their exact stock-runtime boundary is **not
qualified**, because each unchanged control already exposes independent shipping
ABI/provider mismatches. These three artifacts and the camera-extension closure
remain quarantined and are excluded from the production replacement set.

Display's initially missing OEM inputs are now recovered exactly from the pinned
manifest; fresh control02 now compiles and links successfully, but its exact
stock-runtime boundary fails independently on ten CRC edges and the unsigned
import policy. It remains quarantined, not package-qualified. MM sync-fence is unchanged:
unsafe lock-only import is withheld with concrete
deadlock/transaction analysis. No multimedia module package, boot image,
AVB/trust change, flashing, push, release, or physical qualification was performed
by this stage. The released boot-only kernel and all stock partitions remain
unchanged.

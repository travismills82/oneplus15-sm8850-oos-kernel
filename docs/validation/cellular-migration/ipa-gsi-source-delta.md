# DataIPA/GSI `.078` to `.102` bounded-foundation audit

Date: 2026-08-23

The physically validated RMNET-core baseline is unchanged.  This audit compares
the exact OnePlus SM8850 DataIPA source at
`5ab2a689ff87d7d28c511f1762cf41c1b90d965a` (`.078`) with
`d447f713d6403f707a2910383495f4ada98cfa4d` (`.102`).  Only the eleven
runtime source files consumed by the Canoe `gsim`, `ipam`, and `ipanetm`
targets were imported.  The `.102` Bazel rule changes and unrelated targets
were not imported.

No change below is classified as a security fix.  The reviewed source supports
bug-fix, recovery, firmware-lifecycle, memory/lifetime, power/data-path, and
ABI classifications only.

## Runtime changes

| File | Function | Classification | `.102` behavior | Canoe reachability / boundary |
|---|---|---|---|---|
| `gsi/gsi.c` | `gsi_register_device` | SSR/RECOVERY, FIRMWARE LIFECYCLE | Tears down the prior GSI HAL mapping before re-registration. | Called by normal IPA post-init and by re-registration paths. `gsim` and `ipam` must move together. |
| `gsi/gsi.c` | `gsi_deregister_device` | SSR/RECOVERY | Removes the old teardown from deregistration; teardown is paired with the next registration. | Runtime reachable during IPA teardown/recovery. |
| `gsi/gsi.c` | `gsi_status_enabled` | ABI/EXPORT, FIRMWARE LIFECYCLE | Adds a GPL export that reads the GSI enabled state. | Imported by `.102` `ipam`; no other shipped consumer imports it. |
| `ipa.c` | `ipa3_q6_clean_q6_flt_tbls` | SSR/RECOVERY, MEMORY/LIFETIME | Selects atomic/non-atomic allocation mode, records the path, retries allocations, and distinguishes command-construction errors. | Q6 cleanup/SSR path; not an ordinary-data-path-only change. |
| `ipa.c` | `ipa3_q6_clean_q6_rt_tbls` | SSR/RECOVERY, MEMORY/LIFETIME | Same allocation-mode and retry hardening for route-table cleanup. | Q6 cleanup/SSR path. |
| `ipa.c` | `ipa3_q6_set_ex_path_to_apps` | SSR/RECOVERY, MEMORY/LIFETIME | Uses context-appropriate allocation and bounded retries while rebuilding the exception path. | Q6 recovery path. |
| `ipa.c` | `ipa3_post_init` | FIRMWARE LIFECYCLE | Avoids the old GSI 2.2 init step when firmware reports GSI already enabled. | Normal initialization; hardware/version conditional. |
| `ipa.c` | `ipa3_load_ipa_fw` | FIRMWARE LIFECYCLE, SSR/RECOVERY | Queries `gsi_status_enabled`; skips a redundant firmware load and marks the uC loaded when firmware is already active. | Normal/SSR firmware workqueue. This is the sole consumer of the new GSI export. |
| `ipa.c` | `ipa_alloc_pkt_init_ex` | DATA PATH | On IPA 5.5 low-latency producer paths, programs packet-init-ex DPL-disable metadata. | Compiled for Canoe; hardware/client conditional. |
| `ipa.c` | `ipa3_deepsleep_suspend` | BUILD ONLY | Comment correction only. | No runtime change. |
| `ipa_dp.c` | `ipa3_send` | MEMORY/LIFETIME | Removes an unused allocation-flag selection. | Runtime-neutral cleanup. |
| `ipa_dp.c` | `ipa_tx_dp` | STRUCTURAL ABI, DATA PATH | Treats `ipa_tx_meta.pkt_ex_init_valid` as a packet-init-ex request. | Runtime reachable; every shipped binary caller was audited separately. |
| `ipa_dp.c` | `ipa3_lan_rx_pyld_hdlr` | DATA PATH | Handles the decompression packet-status opcode. | Runtime reachable when that hardware status is emitted. |
| `ipa_dp.c` | `ipa3_assign_policy` | DATA PATH | Changes the IPA 5.5 low-latency producer away from the older DMA-only policy. | Compiled for Canoe; hardware/client conditional. |
| `ipa_hdr.c` | `__ipa3_del_hdr_proc_ctx` | MEMORY/LIFETIME | Clears the reverse header/process-context relationship before release. | Runtime reachable during header cleanup. |
| `ipa_hdr.c` | `__ipa3_del_hdr` | MEMORY/LIFETIME | Clears the reverse process-context reference and consistently returns the offset entry. | Runtime reachable during header cleanup. |
| `ipa_hdr.c` | `ipa3_del_hdr_hpc_usr` | MEMORY/LIFETIME, ERROR HANDLING | Repairs reference-count/restoration behavior on partial deletion failures. | Runtime reachable through IPA header-control operations. |
| `ipa_utils.c` | `ipa3_controller_static_bind` | DEVICE-SPECIFIC | Adds IPA 5.2 IoT clock rates. | Not selected by the Canoe non-IoT configuration. |
| `ipa_utils.c` | `ipa3_tag_process` | MEMORY/LIFETIME | Adds bounded allocation/DMA retries and clearer construction errors. | Runtime reachable during tag processing and recovery. |
| `ipa_utils.c` | `_ipa_suspend_resume_pipe` | POWER | Requires a valid mapped endpoint before suspending/resuming coal pipes. | Runtime reachable during suspend/resume. |
| `ipa_utils.c` | static endpoint policy | DATA PATH | Uses a second packet-processing pass for the 5.5 low-latency producer. | Compiled for Canoe; hardware/client conditional. |
| `ipahal_fltrt.c` | `ipahal_rt_generate_empty_img`, `ipahal_flt_generate_empty_img` | MEMORY/LIFETIME, SSR/RECOVERY | Adds bounded coherent-allocation retries for empty route/filter images. | Recovery/initialization paths. |
| `rmnet_ctl_ipa.c` | `ipa3_setup_apps_low_lat_prod_pipe` | DATA PATH | Uses BASIC mode for IPA 5.5 and retains DMA mode for older hardware. | Compiled for Canoe; low-latency control-pipe conditional. |
| `rmnet_ctl_ipa.c` | `ipa_rmnet_ctl_xmit`, `rmnet_ctl_wakeup_ipa` | STRUCTURAL ABI, DATA PATH | Initializes the new packet-init-ex metadata and routes control traffic to the Q6 WAN consumer on IPA 5.5. | Internal to `ipam`; no retained external binary passes the new field. |

## Contract result

- `gsim` imports are unchanged. It adds only `gsi_status_enabled()`.
- `ipam` adds that one GSI import. Its export set is unchanged, but the CRCs of
  `ipa3_ctx`, `ipa3_get_ctx`, `ipa_bridge_tx_dp`, and `ipa_tx_dp` change.
- Only `ipa3_ctx` has a shipped external consumer: `ipanetm`. The `ipanetm`
  source file is unchanged, but the module must be rebuilt against the `.102`
  provider contract.
- No shipped module imports `ipa3_get_ctx`, `ipa_bridge_tx_dp`, or `ipa_tx_dp`.
- The active Peach-v2 WLAN module imports 28 IPA symbols; every symbol and CRC
  remains compatible.
- Stock `usb_f_gsi`, QMI, remoteproc, ramdump, and minidump modules provide
  unchanged imports to `ipam`; they do not consume a changed IPA export.

The minimum contract-complete closure is therefore `gsim` + `ipam` source
upgrade, `ipanetm` provider-contract rebuild, and exact-stock `rmnet_ctl`
re-signing for protected-export trust. Existing controlled-v1 Peach and RMNET
consumers retain their physically validated bytes and signatures.

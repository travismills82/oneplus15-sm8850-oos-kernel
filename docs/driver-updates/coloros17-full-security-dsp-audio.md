# ColorOS 17 SecureMSM/SPU, DSP and audio modernization ledger

Date: 2026-10-30. Initial source:
`0c4d5b23ab7ca52462fd40cb1c9ecb02438a0fe0` on
`experiment/coloros17-sm8850-dlkm-updates`. Target: OnePlus 15 / SM8850,
ColorOS `CPH2745_17.0.0.102(EX01)`, Android 16 vendor/KMI6 contract.

This work owns only the SecureMSM/SPU, DSP and audio source groups. It does
not change common-kernel implementation/configuration, KMI lists, ABI/STG
references, firmware, AVB/load policy, the production branch, or device
partitions. No replacement-module deployment is
claimed by the audit below. Source changes/builds are
recorded explicitly. Matching source names or CRCs do not establish exact
shipping-source identity, and source-only fixes do not protect an unchanged
stock module on the device.

## Ref refresh and selected fixes

Bounded official CodeLinaro `git ls-remote --tags` queries on 2026-10-30
reconfirmed these highest advertised endpoints within the reviewed family.
The recorded tag endpoints are references, not proof of ColorOS source match.

| Subsystem / official repository | Same-family endpoint | Peeled SHA | Selection |
|---|---|---|---|
| [SecureMSM](https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/securemsm-kernel.git) | `LA.VENDOR.16.2.0.r1-21500-kaanapali.0` | `a6e915fd7fed8ca20cec6967c42e8c1dceb02f7e` | Aliases reviewed 21300/21400; review QCE NULL guards only. |
| [SPU](https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/spu-kernel.git) | `LA.VENDOR.16.2.0.r1-21500-kaanapali.0` | `e933e229773b273592eb55e3cfad85fdd215fd80` | Aliases reviewed 21300/21400; no new SPU fix selected merely to update a version. |
| [DSP](https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/dsp-kernel.git) | `LA.VENDOR.16.2.0.r1-21500-kaanapali.0` | `79229a856d0d5064d9f0084497547cd585affb5e` | Aliases reviewed 21300/21400; inspect FastRPC registration state check. |
| [Audio](https://git.codelinaro.org/clo/la/platform/vendor/qcom/opensource/audio-kernel-ar.git) | `AUDIO_HANDSET.LA.11.0.r1-12500-kaanapali.0` | `2bc34f6d8b61628135c9895a9f50e64c7be05b40` | Select 12400 timeout-cleanup fix, not later product-specific DT sleep-vote changes. |

| Group | Original fix / author | Functional scope | Initial status |
|---|---|---|---|
| SEC-01 | `340fcf5a1373745e4949c60488a8ac9721770179`; Om Prakash Singh `<omprsing@qti.qualcomm.com>` | Stop exhausted SG walks in `qce50.c`/`qcrypto.c`; reject NULL final SG before marking it. | Source missing; controls/error-path review pending. |
| DSP-01 | `89e310d94c751816fbc3ec008c8dbe9e6e1a6017`; Adarsh Kumar Sinha `<adarsinh@qti.qualcomm.com>`; references original `b1adc111bd12e6cb6f9eabc21ecc990e1d2d4d01` | Permit registration only after `DSP_CREATE_COMPLETE`, preserving error unlock/ref release. | Source missing; adaptation needed because older unregister context differs. |
| AUD-01 | `4c86a9329b871a685120330d4bb42aed5f9f8b6f`; Vaibhav Raut `<vraut@qti.qualcomm.com>` | On clock-enable `-ETIMEDOUT`, send existing PRM disable operation before returning original failure. | Source missing; provider/config/control proof pending. |

### Control outcomes and isolated changes

The unchanged controls are normal target-specific DDK configurations built
against the exact frozen qualified common-kernel outputs, not a different
Lineage kernel. Actual producer `Module.symvers` files are SDK inputs only;
their modules are not implicitly installed. Every build has a private
`input-manifest.json`, build log and `result.json`. Source changes are kept
separate from the common kernel and configuration.

| Module/control | Build | Shipping comparison | Consequence |
|---|---|---|---|
| QCrypto unchanged `qcrypto-control-run01` | PASS | 12 exports match. All function bodies match, but current `schedule_work()` header changes the stock `system_wq` import to `system_percpu_wq`, which the frozen unsigned-vendor import policy does not permit. | Preserve the actual stock queue explicitly at the two reviewed local sites; do not alter KMI policy. |
| QCrypto queue control `qcrypto-queue-control-run02` | PASS | All 126 function bodies byte-identical to stock, 12 exports, 89 version records and 88 ELF imports identical. Zero missing/CRC/namespace/compiled-policy/protected-export failures. | Local compatibility commit `fd36db8b15813`; SHA256 `48c9daca0e04efcfae270c85fe4c7752586a6ade5c83f5cab1d5b59bbac5cbd7`. |
| QCE unchanged `qce50-control-run01` | PASS | All 19 exports and 22 retained QCE consumer edges match. Exactly 45 functions are byte-identical; only `qce_open` has an extra eight-byte `enable_irq` sequence. | Investigate the IRQ initialization semantics independently of the SG fix. |
| QCE IRQ control `qce50-irq-control-run02` | PASS | All 46 function bodies byte-identical to stock; the 26 mapped KCFI words also match. 19 exports, 77 version records and 76 ELF imports identical. All 22 retained consumer edges match, with zero missing/CRC/namespace/policy/protected failures. | Removing the redundant enable is a real initialization correction, not a signature disguise; separate commit `ddd4de0324cb9`. SHA256 `310b4f25f2bf8fe1f2e75b1ff90775075611e8f848f7e213b4ed41e7468cb45f`. |
| FastRPC unchanged `fastrpc-control-run01` | PASS | Three public exports and three stock EVA import edges match. There are 169 byte-identical functions, but 17 differing functions and ten absent functions associated with additional stock PID/sysfs/tracing and dump behavior. | Do not infer whole-driver shipping identity from the three CRCs. Deployment remains quarantined pending closure of the differences. |
| FastRPC state fix `fastrpc-state-candidate-run02` | PASS | Against unchanged control: only `fastrpc_driver_register` changes; all other 185 function bodies and all 161 mapped KCFI words match. All three export records, 183 version records and 182 imports are unchanged; 12 expanded DWARF types are identical. | Minimal source-only backport `9905eabc27340`; module load is still blocked by pre-existing `system_percpu_wq` policy, not this state guard. SHA256 `dff6e48b1c16681d2f412076bba90c9c68ee0de716b13603e3ee15cd93102627`. |
| QCE SG candidate `qce50-sg-candidate-run03` | PASS | Against the byte-identical IRQ control: all 19 exports, 77 versions and 76 imports unchanged; 22 retained stock consumer edges match; 8 expanded DWARF types and 26 mapped KCFI words identical; zero actual resolution/policy failures. | Official `340fcf5a...` recorded as `ff1f9c0dda81e`; SHA256 `280e2ac37caf1058541cd354a6700c844c4ececbe7a0be4b8cb0885bbfc1bb50`. |
| QCrypto SG candidate `qcrypto-sg-candidate-run03` | PASS | Only `_sha_update` function body changes against the byte-identical queue control; 12 exports, 89 versions and 88 imports unchanged; 8 expanded DWARF types and 109 mapped KCFI words identical; zero actual resolution/policy failures. | Same isolated original fix commit `ff1f9c0dda81e`; SHA256 `10e056cf57ef13d64228f5c03d7e4fbc996131b0a63bcab8710932d6ff62337b`. |
| WCD unchanged `wcd9xxx-control-run01`, final `sdk02` retry | PASS | All 31 function bodies byte-identical to shipping WCD, all 27 mapped KCFI words identical; 5 exports, 33 version records and 32 ELF imports identical; all 11 retained codec consumer edges match. | Full module, not just the clock leaf, is now controlled. SHA256 `b95d9b3a36eb3e3890a1ed8910c8e29ddff1136d06ff7b12568957ec108bda42`. |
| WCD timeout candidate `wcd9xxx-timeout-candidate-run02`, final `sdk02` retry | PASS | Only `audio_ext_clk_prepare` changes (256 to 288 bytes); other 30 function bodies and 27 mapped KCFI words identical. 8 expanded DWARF types identical; 5 exports, 33 versions and 32 imports unchanged; all 11 retained stock codec edges match; zero missing/CRC/namespace/policy/protected failures. | Exact upstream audio timeout fix in `cda06aea6ef78fce38ae6f7cc7fd63449be11a24`; SHA256 `577dbbbb9ab7edbe1cb9792bcc34b0d83b11a501bbb2212a9b62d1775329f6bb`. |

QCrypto's two sites are the bandwidth reaper timer callback and bandwidth
allocation request. Both now use `queue_work(system_wq, &same_work)`;
`INIT_WORK`, cancellation, timer ordering and callback bodies are unchanged.
This preserves the actual stock queue and does not replace every workqueue
operation globally.

The QCE correction removes only `enable_irq()` after successful
`request_irq(..., IRQF_TRIGGER_RISING, ...)`. Canoe's core IRQ is a direct
GIC SPI 274 edge-rising interrupt (`canoe.dtsi`), with no driver
`IRQF_NO_AUTOEN`/descriptor `IRQ_NOAUTOEN` or matching disable. The normal
`__setup_irq()` path in `kernel/irq/manage.c` already starts it; enabling at
depth zero instead emits an unbalanced-enable warning. The source-side
removal therefore preserves normal IRQ startup, registration error handling
and teardown, and restores the shipping instruction-level behavior.

The FastRPC backport applies only the registration-state change from
`89e310d94c751816fbc3ec008c8dbe9e6e1a6017`; its unrelated unregister log
hunk has different newer prerequisite code and is intentionally not
imported. States 0/1/3/4 are rejected while state 2 is accepted, with the
existing channel unlock and acquired-file-reference release on failure.
No callback signature, public structure, UAPI or device creation path changes.
The retained stock `msm-eva.ko` has a 224-byte `cvp_fastrpc_client`
instance; callback relocation offsets `0xd0/0xd8` and probe/callback KCFI
words `0x888d52ed/0x9e546c19` agree with the current driver contract.
Both stock callbacks read `fastrpc_device.handle` at `0x3a0` (928), which
matches unchanged-control DWARF. This constrains the actual client boundary
but does not prove the unrelated stock features absent from this source
can be safely omitted.

The SG change does not touch headers, object allocation, callback prototypes
or source layouts. In QCE it changes the three call paths into which the
guarded count helper is inlined (`qce_ablk_cipher_req`, `qce_aead_req`,
`qce_process_sha_req`), and the other 43 function bodies remain identical.
QCrypto's other 125 function bodies remain identical. Only mapping-symbol
identified `$d.*` four-byte words immediately before callable functions are
counted as KCFI tags: direct-only static functions can have no such tag, so
the previous function's RET must not be misclassified as a changed type hash.

Focused host validation uses
`tools/driver-updates/test_coloros17_security_backports.py`. It compiles
extracted QCE count, FastRPC registration and audio prepare C functions, plus
the exact hash-tail walk/rejection statements, with Clang ASan/UBSan. Five
test groups pass, including five mutation controls that fail when the guards
are deliberately removed/changed. It covers NULL/exhausted SG handling,
registration states and unwind, timeout-only compensation retaining the
original error, and explicit IRQ/queue/lifecycle source contracts. These are
host/local-path tests, not hardware-DMA, actual kernel concurrency, IRQ,
firmware timeout, or physical-device qualification.

### Audio SDK and runtime boundary

The initial audio control/candidate compiled their C objects but failed
modpost for genuine missing dependency outputs. Those failures are retained,
not relabeled PASS. The normal SDK closure was built without source/config
or validation changes:

`q6_pdr -> q6_notifier -> gpr -> spf_core -> adsp_loader -> oplus_audio_daemon
-> audio_prm -> wcd9xxx`, with `snd_event -> gpr` and the existing real
`oplus_mm_kevent_fb` SDK as additional dependencies. Cached qualified SoC
PDR, remoteproc and IPC-logging tables were genuine unmodified producer
outputs. The retry helper used the original private frozen source copies;
it did not import subsequently edited live files. Normal modpost then
completed for both WCD control and candidate. No unresolved-symbol warnings
were suppressed, and no CRC tables were manufactured.

These producers are **SDK-only**, not a request to replace the runtime
audio stack. The candidate still resolves its two PRM APIs against the
exact stock `audio_prm_dlkm.ko`. Its five exports retain their stock CRC,
export class and namespace, and the retained stock codec consumers are
`wcd9378_dlkm`, `wcd938x_dlkm`, `wcd939x_dlkm` (11 import edges).

The private clock object remains 128 bytes; `prm_clk_cfg` remains a 16-byte
member at offset 16, `audio_clk` remains at offset 32, and all nested
`clk_hw`/clock-operation layout is unchanged. The eight checked expanded
types are `audio_ext_clk_priv`, `audio_ext_clk`, `pinctrl_info`, `clk_cfg`,
`clk_ops`, `fw_info`, `firmware_cal`, `wcd_clsh_cdc_info`. Source headers
and firmware packet definitions are unchanged. The only extra operation is
the already-supported PRM clock release on the existing enable-timeout
path, retaining `-ETIMEDOUT` even if that best-effort cleanup fails. Normal
enable, unprepare and no-PRM behavior are preserved. Actual late-ACK and DSP
timeout behavior still requires physical/fault-injection validation.

### Source versus deployment decision

| Group | Source status | Actual candidate module status | Classification / deployment scope |
|---|---|---|---|
| QCE + QCrypto | Official selected NULL checks present; separate stock IRQ/queue compatibility corrections preserved. | Focused compile, true ELF/CRC/namespace/load-policy and controlled private/type/CFI checks pass. | Requires matched modules. Exact QCE/QCrypto candidates are eligible for the integrated experimental module oracle, not a boot-only security claim. |
| FastRPC | Minimal upstream completed-process registration guard present; original UAF-prevention intent retained. | Compile and control-to-candidate layout/CFI/API checks pass; stock feature differences and the inherited forbidden workqueue import remain unresolved. | Unresolved. Source-only improvement; quarantine `frpc-adsprpc.ko` from deployment. Retain stock FastRPC/EVA. |
| WCD clock | Exact official timeout compensation present, without changing the PRM API or packet format. | Focused complete module build and actual retained-provider/consumer/type/CFI checks pass. | Requires matched modules. Only WCD is a proposed replacement; SDK PRM/GPR/codec modules remain stock. |
| SPU | No independent coherent missing fix selected from the reviewed unchanged same-family tip. | No SPU source or runtime module changed. | Not applicable to these three selected patch groups; no claim that every SPU change is already applied, and no forced refresh merely to change a version. |

Full integrated 1,070-location population/provider-precedence validation is
a separate gate below. The per-module results
above are not a substitute for that gate, AVB trust/packaging, or physical
qualification. No final partition image or boot artifact is produced by
this source group, and every physical test is **NOT TESTED**.

None of these selected fixes changes the deliberate idle waits in
`adci_thread`, `zram_comp`, or `hfi_core_dbg_cl`. The existing common-kernel
watchdog protections/filter are unchanged. In particular an audio PRM clock
timeout repair is not evidence of an ADCI Secure World, QPACE, or HFI IPC
hang being fixed.

## Integrated four-provider population gate

The private experiment
`security/integrated-oracle-rmnet-qce-qcrypto-audio-run01/` uses the exact
stock 1,070-location population, replacing only `vendor_dlkm/lib/modules/`
`rmnet_core.ko`, `qce50_dlkm.ko`, `qcrypto-msm_dlkm.ko`, `wcd9xxx_dlkm.ko`.
Both consumer and provider inputs are this same hybrid population, so a
rebuilt module's own imports are checked rather than substituting its old
stock consumer record. Neither SDK dependency modules nor quarantined
FastRPC/GPU/camera/video source candidates are included.

| Gate | Observed result |
|---|---|
| Module locations | 1,070; exactly four changed, all other 1,066 modules byte-identical to stock |
| Active compatible / dormant / blockers | 1,032 / 38 / 0 |
| Hard ELF imports | 60,701 / 60,701 resolved |
| All undefined ELF imports / weak | 60,707 / 6 |
| CRC records | 61,777 / 61,777 match |
| Missing / CRC / namespace failures | 0 / 0 / 0 |
| Actual vmlinux-first provider edges | 54,785 checked, zero production mismatches |
| Actual compiled unsigned-import policy | 54,986 edges, zero failures; compiled 10,381 permitted imports and 633 protected exports |
| Protected-export collisions | 0 |
| Stock ambiguous-provider edges | All 3,220 edges and provider records unchanged; zero new/changed ambiguity |
| Replacement-provider edges | 89, each with one actual provider: RMNET 56, QCE 22, WCD 11; QCrypto has zero external export-import edges |
| Dependency/load metadata | Every stock `modules.*` file byte-identical |
| SDK provider substitutions | 0 |

QCrypto's zero external export-import edges do not imply that its algorithms
are unused: its existing module-init registrations are consumed through the
kernel crypto API. The stock archived runtime lists all four modules loaded.

Each of the four canonical names occurs **only once** in the complete stock
module population, in `vendor_dlkm`; none has a competing `system_dlkm` or
`vendor_boot` copy. The actual stock `vendor.img` EROFS directory/inode data
also shows `/lib/modules` is a symlink to `/vendor_dlkm/lib/modules`, so
`/vendor/lib/modules` in `modules.dep` is the same provider location, not an
old hidden stock copy. Neither the normal nor recovery `vendor_boot`
`modules.load` lists requests these four modules. Thus this package changes
their normal Android providers and does not pretend to replace a second
recovery copy. A fresh recovery/device runtime load trace remains **NOT
TESTED**.

For all four actual stock ext4 file inodes, UID/GID are `0:0`, mode is
`0644`, and the exact SELinux xattr is `u:object_r:vendor_file:s0`.
Unprivileged host extraction ownership is not packaging authority.

The four-provider oracle is a complete static module-contract gate, not a
claim that a new DLKM filesystem is authorized by the stock parent vbmeta,
fits actual live LP metadata, or has passed physical tests. Those gates
remain separate.

Official patches are cached outside source under
`out/coloros17-driver-audit-reference/patches/`. Original provenance,
dependencies and any local adaptation must remain individually reviewable;
the root integration owner performs commits.

## Exact affected stock modules and build ownership

All modules below are stock `vendor_dlkm/lib/modules/` files, without BTF or
DWARF. They remain unchanged on the physical device. The authoritative
[inventory](coloros17-module-inventory.md) records signatures/dependencies
and distinct normal/recovery locations.

| Stock module | SHA256 | Selected source/build target and immediate dependency |
|---|---|---|
| `qce50_dlkm.ko` | `781157507103566ebd2404f24c436eec4e03d856982be1d3fc939cf13fbcbd52` | `securemsm-kernel:canoe_perf_qce50_dlkm`; source `crypto-qti/qce50.c`; stock `sps_drv`. |
| `qcrypto-msm_dlkm.ko` | `ccb1d347f03c1b97f4534c788a1c111854cd9a89f9542ca3f23465c6a2b776e3` | `securemsm-kernel:canoe_perf_qcrypto-msm_dlkm`; source `crypto-qti/qcrypto.c`; stock QCE provider. |
| `frpc-adsprpc.ko` | `b4063970bd3886e238e148a76de544b43bf1194bf2abd0b3a9d73c273d72c609` | `dsp-kernel:canoe_perf_frpc-adsprpc`; FastRPC/RPMSG/sysfs objects; `qcom-scm,mem_buf_dev,pdr_interface,qcom_glink`. |
| `wcd9xxx_dlkm.ko` | `9d9e6534e65d8c93655e789ddc1bf2f36d062a8979b957567ee24d434d032567` | `audio-kernel:canoe_perf_wcd9xxx_dlkm`; includes `audio-ext-clk-up.c`, not a separate clock module; stock `audio_prm_dlkm`. |

All target packages above are rooted at `//vendor/qcom/opensource/`.
QCE also serves retained `qcedev-mod_dlkm.ko`; a new QCE provider must remain
compatible with that stock consumer. FastRPC's `fastrpc_driver` objects and
callbacks are used by EVA clients, not just userspace IOCTLs. Audio's
`clk_cfg`/PRM/GPR boundary and codec consumers must be checked, without
silently rebuilding/deploying their dependency modules.

## Direct shipping observations, not source-match assumptions

- Stock FastRPC `fastrpc_driver_register` at `0x6ec8` reads process state;
  `0x6ecc` compares it to 3 and branches to registration when less than 3.
  The current enum is default 0, create-start 1, create-complete 2,
  exit-start 3. The source has the same exit-only rejection and can register
  before its device pointer exists. Stock probe-callback KCFI check is
  `0x888d52ed`; driver device/handle/probe offsets observed are
  `0xc0/0xc8/0xd0`. Control DWARF and actual callback consumers remain a gate.
- Stock `wcd9xxx_dlkm` `audio_ext_clk_prepare` at `0x60c` calls existing
  `audio_prm_set_lpass_clk_cfg(...,1)` and reaches negative-error return
  `0x658` without any compensation call. This also proves that the PRM
  path, rather than the no-PRM stub, is compiled into the shipping module.
  The same provider's disable operation already serves normal unprepare;
  the proposed fix introduces no new firmware command/API.
- QCE/QCrypto SG source walks have the selected missing guards. QCE's
  `count_sg` is inlined in shipping code, so its complete call paths require
  focused instruction/control analysis rather than a guessed function name.
  The upstream QCE stop-on-NULL guard returns a partial count; it is not
  claimed to validate every malformed SG request or DMA length.

Initial source hashes (before any owned implementation edit):

| File | SHA256 |
|---|---|
| `securemsm-kernel/crypto-qti/qce50.c` | `2fc5b22030f8763b54c877c07d129e22de2602ad692262c2c9e9d7d9ba7e8eaf` |
| `securemsm-kernel/crypto-qti/qcrypto.c` | `96f0494b3ff0b717bcb9a5ed64af6f40aba69562838434d884d638485f4cfeb2` |
| `dsp-kernel/dsp/fastrpc.c` | `2e934713a026546f63acdb18080e49d980b08a4377a29d393f6b01f85ccfe38a` |
| `audio-kernel/asoc/codecs/audio-ext-clk-up.c` | `b164d02f5a575ce8dd40ff49740278d2e5ea3a536c14d78099af512d1aa36e09` |

## Qualification ledger

Unchanged-control builds are coordinated centrally; no competing full
kernel/Bazel build is started here. Private evidence belongs under
`/home/travis/Android/kernel-test-backups/coloros17-full-modernization-20261030/security/`.

For each group, the required sequence is source/stock evidence, unchanged
control, actual private/export/import/KCFI/load-policy review, minimal source
fix, focused candidate build, and the complete affected stock provider graph.
Any header/API, toolchain, private-layout or unavailable firmware prerequisite
must be reported explicitly rather than hidden by CRC success. Build/static
results and physical deployment status are separate. All runtime tests are
currently **NOT TESTED**; no partition images or release artifacts are made.

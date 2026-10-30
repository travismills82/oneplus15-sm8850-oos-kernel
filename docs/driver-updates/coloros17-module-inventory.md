# ColorOS 17.0.0.102 exact stock-module inventory

Audit date: 2026-10-30. Binary inventory only; no module update or physical test is claimed.

Generated from the exact released .114 security source's stock oracle and the original .102 binary set. The stock framework is Android 17 / SDK 37; vendor is Android 16 / SDK 36. This is a mixed platform contract, not proof of another firmware's compatibility.

## Population

| Partition | Files |
|---|---:|
| system_dlkm | 103 |
| vendor_dlkm | 461 |
| vendor_boot | 506 |
| Total | 1070 |

There are 702 distinct canonical names, not 1070 distinct drivers. 366 names have copies in multiple partitions; 2 of these have different binary bytes. Copies must remain partition-qualified. The frozen normal/recovery load contract has 1032 compatible entries and 38 dormant entries; these are not the observed 691 runtime-loaded modules.

The two unequal-name groups are zram and zsmalloc. Each has a signed system_dlkm GKI variant and matching unsigned vendor_dlkm/vendor_boot QCOM variants. A name-only replacement/provider model would conceal this difference. The other cross-partition copies are byte-identical; do not assume that permits changing only one normal/recovery load path.

[Complete 1070-row inventory](coloros17-module-inventory.tsv) includes canonical name, binary hash/size, signature metadata, vermagic, version/source hash metadata, imports, exports, build ID, private type-section availability, source/build-file/repository candidates, explicit stock-source confidence, firmware declarations/dependency confidence, private-header review status and load classification.

## Source ownership and confidence

| Review bucket | Partition-qualified files |
|---|---:|
| AUDIO | 46 |
| BLUETOOTH | 13 |
| CAMERA | 8 |
| DATAIPA_RMNET_NETWORK | 85 |
| DISPLAY_DRM | 12 |
| DSP | 13 |
| GPU | 8 |
| MEMORY_DMA_IOMMU_SYNC | 70 |
| OPLUS_OTHER | 177 |
| PLATFORM_OR_GKI_OTHER | 382 |
| POWER_THERMAL | 78 |
| SECUREMSM_SPU | 28 |
| TOUCH_FINGERPRINT | 63 |
| UFS_STORAGE | 9 |
| USB_TYPEC | 53 |
| VIDEO_HFI | 6 |
| WLAN | 19 |

Buckets route review; they are not exact source attribution. Source/build-file name associations are explicitly candidates. All 1070 stock source revisions remain EXACT_REVISION_NOT_VERIFIED: a matching filename, release tag, vermagic, srcversion, export CRC or source-equivalent function does not prove a matching shipping implementation. Do not apply historical OxygenOS .601 source lineage as ColorOS .102 binary provenance.

Every row explicitly marks original_source_repository=UNVERIFIED_FOR_STOCK_BINARY. source_repository_candidates maps existing local source/build-path candidates to manifest-declared Google Common, OnePlus SoC or OnePlus modules repositories and the official CodeLinaro URLs previously audited in [the ref inventory](coloros17-upstream-refs.tsv). Repository ownership/availability is verified at that reference level, not as the origin of the stock binary. Rows without a local path association retain UNKNOWN. Generic build lists can yield multiple candidates; neither a candidate URL nor its release tag is a source-match or security-applicability claim.

Firmware: 3 partition-qualified module records have explicit ELF modinfo firmware declarations; their exact declared strings are preserved per row. Other rows use NOT_DECLARED_NOT_PROOF_NONE. firmware_dependency_status remains unverified for every module: MODULE_FIRMWARE metadata need not enumerate dynamically requested firmware, secure-world dependencies, calibration, HAL/user-space contracts or DT-selected files. No firmware dependency is inferred absent and no firmware compatibility is approved by this inventory.

Private headers: every row retains private_header_dependency_status=UNVERIFIED_EXACT_SOURCE. Applicable RMNET and DWC3 rows link related partial manual boundary reviews; these are not complete per-module include/dependency closures or proof of shipping layout equivalence. Other rows explicitly state NO_PER_MODULE_PRIVATE_HEADER_REVIEW. BTF availability, matching export CRCs and name-based public source associations cannot establish private-header compatibility; complete dependency/config/layout review remains a replacement-module gate.

All 103 GKI modules record 6.12.69-android16-6-g242e1a07f878-ab15865446-4k and scmversion g242e1a07f878. All 967 vendor/vendor_boot files record 6.12.69-android16-6-o-gb79517a1cbef-4k; 491 of those copies additionally record scmversion g4808b15dc936. These are source-search anchors, not proof of a complete source/config/toolchain match. Per-binary compiler-comment metadata is preserved where available.

All 1070 compiler comments identify Android clang 19.0.1, build 14043575 / r536225, LLVM commit b3a530ec6537146650e42be89f1089e9a3588460. Rust Binder additionally identifies rustc 1.82.0-dev (f6e511eec 2024-10-15), Android Rust toolchain linux-glibc-15545352. A replacement toolchain must be reviewed for compiler-generated Rust identity, CFI, DWARF and module ABI effects rather than chosen only by version proximity.

Private type evidence: 105 ELF files contain .BTF; 0 contain .debug_info. The complete section inventory records availability per exact binary; absent BTF/DWARF is not evidence of layout compatibility. All binaries are uncompressed AArch64 ELF inputs.

## Import, provider and load metadata

The independent loader-oriented parser counts 60707 named ELF undefined symbols: 60701 hard and 6 weak. All 61777 MODVERSIONS records are accounted for, including special module-layout entries. Export records come only from actual __ksymtab/__ksymtab_gpl and associated CRC/namespace tables, never arbitrary global ELF definitions. This inventory is not a replacement for the complete candidate provider oracle.

Companions: [actual exports](coloros17-module-exports.tsv), [ELF sections](coloros17-module-sections.tsv), [parameters](coloros17-module-parameters.tsv), [ELF dependency names](coloros17-module-dependencies.tsv), [depmod paths/order](coloros17-module-depmod-paths.tsv), [ordered stock load requests](coloros17-module-loads.tsv), [metadata hashes/presence](coloros17-module-loading-metadata.tsv), [cross-partition copies](coloros17-module-duplicates.tsv), and [raw stock images](coloros17-stock-images.tsv). modules.dep uses explicit system/vendor/ramdisk paths; module names alone do not decide runtime precedence. Metadata missing from the extracted baseline is marked NOT_CAPTURED_OR_ABSENT, not asserted absent from the original partition or synthesized.

In the depmod-paths TSV, dependency_loader_paths_in_order=NONE explicitly represents an empty dependency list. Decode NONE to [] before interpreting loader paths; otherwise split the field on whitespace in its recorded order. This preserves a rectangular four-column table without a trailing empty field/tab. NONE is a reserved encoding, not a module request; a literal matching input is rejected rather than silently conflated. This changes only the audit representation, never stock modules.dep bytes or dependency semantics.

## Signing and deployment gates

All 103 system_dlkm files are trusted-signed stock GKI modules; all 967 vendor_dlkm/vendor_boot files are retained unsigned under the qualified Android policy. Actual signed-module metadata uses PKCS#7 / SHA256 and public key identifier 7B:47:78:24:F0:27:CB:C7:E5:84:37:5F:41:33:60:F2:C0:96:2E:BC. Signature metadata is inventory, not a fresh cryptographic signature-verification claim. Current configuration keeps MODULE_SIG, MODULE_SIG_PROTECT, MODVERSIONS, GENDWARFKSYMS and trimming; MODULE_SIG_FORCE is not enabled, as in the qualified firmware contract. The current production signing-hash setting is sha1; do not confuse it with the actual stock module signature algorithms listed individually.

The stock private signing key is not provided by the public certificate. Rebuilt protected GKI providers require a legitimately trusted signing/dependency closure. Unsigned vendor providers remain subject to permitted imports, protected exports, namespaces and CRCs. Do not re-sign retained stock files, spoof vermagic/CRCs, force loading or disable validation. A new matched-module deployment needs a new kernel/module/AVB/package qualification; existing boot-only release artifacts stay unchanged.

The Downloads raw system_dlkm/vendor_dlkm/vendor_boot hashes match the previously protected stock partitions. The baseline images named *-verity.img are filesystem-only mounted views and have different lengths/hashes; they are not interchangeable raw partition payloads. No binaries, firmware, private signing keys or raw device property dump are committed here.

## Reproduction

```sh
python3 tools/driver-updates/coloros17-module-inventory.py \
  --baseline <immutable-.102-baseline> \
  --oracle <frozen-five-CVE-static-oracle-01> \
  --firmware-images <CPH2745IN_17.0.0.102> \
  --output-dir docs/driver-updates
```

Use --check-only for an output-free repeat of all binary/hash/count checks. No property dump is consumed or published by the generator. It never reads a serial, PIN, credential or private key. No device is accessed and no source revision is inferred from a release name.

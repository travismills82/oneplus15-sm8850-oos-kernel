# Audio .059 selective GPR static validation

## Result

`PASS — READY FOR PHYSICAL TEST`

The only source/runtime change is the `.059` GPR remove-path ordering change
that synchronously cancels `notifier_reg_work` before GPR notifier/device
teardown.  It is teardown work-race/lifetime hardening, not a claimed complete
UAF or security fix.

The external DDK build inherited the qualified kernel contract:

- kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- `.config`: identical
- `Module.symvers`: identical
- Image functional contract: identical
- vmlinux functional contract: identical
- ABI report: empty
- KMI: pass
- controlled-v1 signing identity: unchanged

## Delivery closure

`gpr_dlkm.ko` is the only source replacement.  Its export set and CRCs are
identical to the baseline.  Its only import addition is
`cancel_work_sync=0x35480a5b`, resolved by the qualified vmlinux.

`CONFIG_MODULE_SIG_PROTECT` makes the complete delivery/signing closure 23
modules: GPR plus 22 exact-stock source consumers.  Those consumers are
controlled re-signs, not Audio `.059` source upgrades.  To fit this protected
closure into the fixed production filesystem geometry, `wcd9378_dlkm` and
`wcd939x_dlkm` have only unneeded non-allocating ELF symbols removed before
signing.  Validation proves their allocated sections, imports, exports, CRCs,
modinfo, and runtime payload contract are unchanged.  The filesystem retains
its stock notice content, `lost+found`, size, inode labels, and AVB/FEC
geometry.

Final validation:

- vendor modules: 436
- source replacements: 1
- exact-stock protected closure re-signs: 22
- unexpected module changes: 0
- unresolved imports: 0
- CRC mismatches: 0
- protected-export failures: 0
- signature failures: 0
- external signed-provider edges: 0
- system `modules.load`: 46 entries
- `wwan.ko`: entry 21
- ext4: pass
- partition-local AVB hashtree/FEC/footer: pass

## Candidate

- image: `out/audio-gpr-059-candidate/vendor_dlkm.img`
- SHA-256: `bb005e764ccfc3af7eec9a73f291a85a44d966478b73ee480617003ae44b079b`
- base vendor_dlkm: `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2`
- boot: unchanged
- system_dlkm: unchanged
- vendor_boot: unchanged
- VBMeta: unchanged
- physical qualification: PASS for normal-runtime compatibility on 2026-08-23;
  `gpr_remove()` teardown path not observed

The exact tested payload is frozen by
`physical-validation-2026-08-23.md`. Graphics is no longer order-gated by the
Audio candidate.

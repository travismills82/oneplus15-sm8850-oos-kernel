# Platform `.099.086` selective CoreSight ETR static validation

Date: 2026-08-23

## Decision

`READY FOR PHYSICAL TEST`

The full Platform `.099.086` generation is not imported. The only candidate
change is official commit
`db1b06b53dcf37388f95105123ba36a854724d34`, which adds the active SYSFS ETR
buffer lifetime guard to `coresight_tmc.ko`.

## Frozen input baseline

- canonical branch: `feature/controlled-v1-audio059-graphics057-camera073-rer`
- canonical source commit: `a83e6fbd78e30d7abc5223b69fbdf3afb02f0b2c`
- kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- boot SHA-256:
  `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab`
- system-DLKM SHA-256:
  `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef`
- Camera-qualified vendor-DLKM SHA-256:
  `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`

## Candidate

- path: `out/platform-coresight-etr-086-candidate/vendor_dlkm.img`
- SHA-256:
  `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`
- physical qualification: `NOT PERFORMED`
- boot changed: `no`
- system-DLKM changed: `no`
- vendor-DLKM changed: `yes`
- intended module changes: `1`
- unexpected module changes: `0`

## Kernel and module contracts

- kernel-contract guard: `PASS`
- `.config`: `IDENTICAL`
- `Module.symvers`: `IDENTICAL`
- Image functional contract: `IDENTICAL`
- vmlinux functional contract: `IDENTICAL`
- dist: `PASS`
- ABI: `PASS / EMPTY`
- KMI: `PASS`
- vendor module inventory: `436`
- protected-signing closure: `coresight_tmc.ko` only
- import contract changes: `0`
- export contract changes: `0`
- unresolved imports: `0`
- CRC mismatches: `0`
- protected-export failures: `0`
- signature failures: `0`
- controlled-v1 signer: `PASS`
- vermagic: exact frozen `g6744` release
- system `modules.load`: `46` entries
- `wwan.ko`: entry `21`
- ext4: `PASS`
- partition-local AVB hashtree/footer: `PASS`

The kernel-build aquery contains 101,648 action inputs and does not consume
`coresight-tmc-etr.c`; that file is consumed only by the external DDK action.
The machine-code comparison proves one inlined CoreSight mode load in the
qualified module and two in the candidate, with the added load checking
`CS_MODE_SYSFS`. No vermagic or binary patching is used.

## Physical test plan

Use the hardened TWRP controlled-stack helper in dry-run mode first. Back up
and verify the installed vendor-DLKM, then write only the candidate
vendor-DLKM and require an exact read-back SHA-256 before booting. Do not
rewrite boot, system-DLKM, vendor_boot, or VBMeta.

After boot, prove the frozen kernel release and the packaged
`coresight_tmc.ko` runtime hash, signer, and vermagic. Confirm the module loads
without symbol, CRC, signing, protected-export, or duplicate-registration
failures. Inventory the actual CoreSight sources and ETR sinks. Exercise a
SYSFS ETR trace only if the active source/sink and safe control sequence are
positively identified; do not inject a tracing fault. If the vulnerable
buffer-resize condition is not safely reached, record `TRIGGER NOT OBSERVED`.

Perform two clean boots and regression-check Camera, Graphics/UI, Audio,
WLAN, cellular/RMNET, Bluetooth, NFC, suspend/resume, and USB. Scan full logs
for CoreSight/TMC/ETR failures, use-after-free, refcount, lock, IOMMU/SMMU,
Oops, BUG, KASAN, UBSAN, panic, and call traces.

Rollback is the exact Camera-qualified vendor-DLKM SHA-256
`8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`.
Verify its partition read-back hash before rebooting after any rollback.

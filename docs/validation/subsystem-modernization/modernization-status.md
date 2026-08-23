# Controlled subsystem modernization status

The canonical input is
`feature/controlled-v1-audio059-graphics057-camera073-rer`. Its
physically qualified payload hashes remain authoritative; an audit or build
result does not advance this baseline without a separate physical pass. The
tested Camera source lineage is `e1d4b2b61c6c8f4fb8e3afe9bb9d187c83bc7fbf`
and the exact vendor-DLKM SHA-256 is
`8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`.

| Subsystem | Current | Candidate | Runtime Value | Static | Physical | Decision |
|---|---|---|---|---|---|---|
| Audio | `.046` | `.059` GPR teardown only | Pending-work cancellation before GPR teardown | PASS | PASS normal runtime; teardown path not observed | QUALIFIED |
| Graphics | `.038` | `.057` secure-guard unlock handling only | Memory-ownership hardening on secure guard-page teardown; broad `.057` GMU/DT/firmware changes rejected | PASS | PASS normal runtime; unlock-failure branch not observed | QUALIFIED |
| Display | `.071` | `.097` | Active AA601 panel files unchanged; remaining DCP-HFI/DRM/fence generation needs matched firmware/HAL and PLZ110 DT is foreign | NONE | NOT TESTED | DEFERRED |
| Camera | `.061` | `.073` RER snapshot only | Userspace-shared RER flash command race/bounds hardening; broad `.073` camera, firmware, HAL and foreign DT generation rejected | PASS | PASS normal runtime; RER command path not observed | QUALIFIED |
| Platform | `.099.064` | `.099.086` CoreSight ETR guard only | Active SYSFS ETR buffer lifetime hardening; broad platform, scheduler, storage, VM, IPC, power, and foreign-device changes rejected | PASS | NOT TESTED | READY FOR PHYSICAL TEST |

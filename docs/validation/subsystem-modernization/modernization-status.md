# Controlled subsystem modernization status

The canonical input is
`feature/controlled-v1-audio059-graphics057-camera073-platform086-coresight`.
Its physically qualified payload hashes remain authoritative; an audit or
build result does not advance this baseline without a separate physical pass.
The tested Platform source lineage is
`871e53d405b156df2584300fc3b80be5b0fc32bb` and the exact vendor-DLKM
SHA-256 is
`dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`.

| Subsystem | Current | Candidate | Runtime Value | Static | Physical | Decision |
|---|---|---|---|---|---|---|
| Audio | `.046` | `.059` GPR teardown only | Pending-work cancellation before GPR teardown | PASS | PASS normal runtime; teardown path not observed | QUALIFIED |
| Graphics | `.038` | `.057` secure-guard unlock handling only | Memory-ownership hardening on secure guard-page teardown; broad `.057` GMU/DT/firmware changes rejected | PASS | PASS normal runtime; unlock-failure branch not observed | QUALIFIED |
| Display | `.071` | `.097` | Active AA601 panel files unchanged; remaining DCP-HFI/DRM/fence generation needs matched firmware/HAL and PLZ110 DT is foreign | NONE | NOT TESTED | DEFERRED |
| Camera | `.061` | `.073` RER snapshot only | Userspace-shared RER flash command race/bounds hardening; broad `.073` camera, firmware, HAL and foreign DT generation rejected | PASS | PASS normal runtime; RER command path not observed | QUALIFIED |
| Platform | `.099.064` | `.099.086` CoreSight ETR guard only | Active SYSFS ETR buffer lifetime hardening; broad platform, scheduler, storage, VM, IPC, power, and foreign-device changes rejected | PASS | PASS normal runtime; ETR resize trigger not observed | QUALIFIED |

The ordered modernization audit is complete. Display remains deliberately
deferred because the candidate generation requires a matched device-specific
firmware, HAL, and DT contract; the other bounded candidates are physically
qualified at their explicitly recorded coverage boundaries.

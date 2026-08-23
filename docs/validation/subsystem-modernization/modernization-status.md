# Controlled subsystem modernization status

The canonical input is
`feature/controlled-v1-wlan053-bt046-nfc102-cellular102-audio059-gpr`. Its
physically qualified payload hashes remain authoritative; an audit or build
result does not advance this baseline without a separate physical pass. The
tested Audio source lineage is `a310804ef75d48723e37a8ae515e4ea557b7dd59` and
the exact vendor-DLKM SHA-256 is
`bb005e764ccfc3af7eec9a73f291a85a44d966478b73ee480617003ae44b079b`.

| Subsystem | Current | Candidate | Runtime Value | Static | Physical | Decision |
|---|---|---|---|---|---|---|
| Audio | `.046` | `.059` GPR teardown only | Pending-work cancellation before GPR teardown | PASS | PASS normal runtime; teardown path not observed | QUALIFIED |
| Graphics | `.038` | `.057` secure-guard unlock handling only | Memory-ownership hardening on secure guard-page teardown; broad `.057` GMU/DT/firmware changes rejected | PASS | NOT TESTED | READY FOR PHYSICAL TEST |
| Display | `.071` | `.097` | Not audited | NOT STARTED | NOT TESTED | WAITING |
| Camera | `.061` | `.073` | Not audited | NOT STARTED | NOT TESTED | WAITING |
| Platform | `.099.064` | `.099.086` selective only | Not audited | NOT STARTED | NOT TESTED | WAITING |

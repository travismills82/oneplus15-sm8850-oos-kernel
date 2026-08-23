# Controlled subsystem modernization status

The canonical input is
`feature/controlled-v1-audio059-graphics057-secure-guard`. Its
physically qualified payload hashes remain authoritative; an audit or build
result does not advance this baseline without a separate physical pass. The
tested Graphics source lineage is `a2d512ad779b082749ca10879b7b6d71aca145b6`
and the exact vendor-DLKM SHA-256 is
`a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`.

| Subsystem | Current | Candidate | Runtime Value | Static | Physical | Decision |
|---|---|---|---|---|---|---|
| Audio | `.046` | `.059` GPR teardown only | Pending-work cancellation before GPR teardown | PASS | PASS normal runtime; teardown path not observed | QUALIFIED |
| Graphics | `.038` | `.057` secure-guard unlock handling only | Memory-ownership hardening on secure guard-page teardown; broad `.057` GMU/DT/firmware changes rejected | PASS | PASS normal runtime; unlock-failure branch not observed | QUALIFIED |
| Display | `.071` | `.097` | Audit may proceed from the exact Graphics-qualified baseline | NOT STARTED | NOT TESTED | AUDITING |
| Camera | `.061` | `.073` | Not audited | NOT STARTED | NOT TESTED | WAITING |
| Platform | `.099.064` | `.099.086` selective only | Not audited | NOT STARTED | NOT TESTED | WAITING |

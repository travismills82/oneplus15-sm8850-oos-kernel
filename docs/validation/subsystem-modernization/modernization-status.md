# Controlled subsystem modernization status

The canonical input is
`feature/controlled-v1-wlan053-bt046-nfc102-cellular102-core` at
`0913a3b6c8549ecf8d311803cfe4f49da60393a5`.  Its physically qualified payload
hashes remain authoritative; an audit or build result does not advance this
baseline without a separate physical pass.

| Subsystem | Current | Candidate | Runtime Value | Static | Physical | Decision |
|---|---|---|---|---|---|---|
| Audio | `.046` | `.059` GPR teardown only | Pending-work cancellation during active GPR transport teardown | PASS | NOT TESTED | READY FOR PHYSICAL TEST |
| Graphics | `.038` | `.057` | Not audited; order gate held by Audio candidate | NOT STARTED | NOT TESTED | WAITING |
| Display | `.071` | `.097` | Not audited | NOT STARTED | NOT TESTED | WAITING |
| Camera | `.061` | `.073` | Not audited | NOT STARTED | NOT TESTED | WAITING |
| Platform | `.099.064` | `.099.086` selective only | Not audited | NOT STARTED | NOT TESTED | WAITING |

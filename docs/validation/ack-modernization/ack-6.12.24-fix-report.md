# ACK 6.12.24 fix report

The official Android Common Kernel 6.12.23 to 6.12.24 range contains 394
ordered commits. This report groups the imported range by changed subsystem
and commit subject; it does not claim that every fix is runtime-reachable on
Canoe. Exact commits and dispositions are in
`ack-6.12.23-to-6.12.24-commits.tsv`.

| Classification | Commits | Scope |
|---|---:|---|
| Security | 0 separately asserted | No CVE or security claim is inferred solely from a stable commit subject. |
| Memory safety | 19 | Bounds, NULL, leak, overflow, double-free, and race/UAF corrections across common drivers and subsystems. |
| Filesystem | 31 | F2FS corruption/bounds handling, SMB/NFS, EROFS, and other filesystem fixes. |
| Networking | 44 | Core networking, Bluetooth, qdisc, protocol, and driver fixes. |
| Scheduler | 12 | Cgroup/cpuset and scheduling correctness fixes. |
| Power | 15 | Runtime PM, suspend, regulator, and power-domain corrections. |
| USB | 4 | USB core/driver correctness fixes. |
| Storage | 14 | Block, SCSI, and storage-driver corrections. |
| Driver/core | 254 | Architecture, driver, tooling, and kernel-core stable fixes. |
| Android-specific integration | 1 | Point-release integration/version boundary. |

Notable memory-safety subjects in the official range include an HSI
`ssi_protocol` race/UAF correction, an s390 PMU error-path double free, Venus
HFI bounds validation, and multiple NULL/overflow guards. These descriptions
are deliberately narrower than a security advisory: applicability depends on
the Canoe configuration and runtime hardware path.

The point release also carries F2FS validation fixes, networking and Bluetooth
initialization corrections, cpuset teardown/race fixes, EROFS error handling,
and a broad set of device-driver fixes. Already-present OnePlus changes were
content-checked and not applied twice. Two Android integration cases required
manual review; their resolutions are documented in the main audit.

No 6.12.25 commit is present. No vendor subsystem source, firmware, userspace,
device-tree, or runtime package generation was updated by this experiment.

# Repository documentation

This directory contains project-owned documentation, release records, security audits, and validation evidence for the OnePlus 15 SM8850 OxygenOS 16 kernel project.

## Documentation map

| Path | Purpose |
| --- | --- |
| [`controlled-oos-signing-v1.md`](controlled-oos-signing-v1.md) | Controlled OxygenOS module-signing design and procedure. |
| [`controlled-stack-release-process.md`](controlled-stack-release-process.md) | Controlled-stack release and validation procedure. |
| [`security/`](security/) | Security audits and CVE applicability records. |
| [`releases/`](releases/) | Per-release manifests, release notes, and validation sign-offs. |
| [`validation/`](validation/) | Subsystem qualification, source-delta, contract, and physical-validation evidence. |

## Security

The current 2026 kernel security applicability/provenance audit is:

- [`security/cve-audit-2026.md`](security/cve-audit-2026.md)

## Validation data

Subsystem validation material is grouped under [`validation/`](validation/). Machine-readable manifests that document qualification results belong beside the matching subsystem validation records rather than in the repository root.

## Kernel-native documentation

Documentation that is part of the Linux/Android kernel source tree is intentionally **not** moved here. Files under paths such as `kernel_platform/common/Documentation/` and documentation stored next to drivers or subsystems are part of the upstream/source layout and may be referenced by kernel tooling, maintainers, or build processes.

This `docs/` directory is for documentation owned by this repository and its release/validation process.

# ColorOS .201 / Linux 6.12.119 maintenance audit — 2026-11-24

Base: `e536034c49674f4efdfe6003d556b22552aca36a` (common tree
`2d61ff4667d2678cc76c6e474609c39f29db8663`); CPH2747, ColorOS
17.0.1.201(EX01), Android 16 ACK KMI6, 4 KiB pages. Work is isolated on
`experiment/coloros17-linux612119-security-maintenance`. The release
architecture remains boot-only with original ColorOS DLKM. Nothing here
constitutes a change to the firmware security-patch level or handset state.
For the completed offline gates, exact tested image, and post-unlock results,
see the [physical canary report](coloros17001201-linux-6.12.119-maintenance-canary-2026-11-24.md).

## New built-in kernel backports

| CVE | Upstream fix / reference branch | Local adaptation |
| --- | --- | --- |
| [CVE-2026-99652](https://www.cve.org/CVERecord/?id=CVE-2026-99652) | Chris Mason; mainline `d03df42ddae196f20e9cfb775704e60269eb6b44`, Linux 6.18 `1acdf007989ffe56af434998f0d330e8473d1b07` | `CONFIG_USERFAULTFD=y`; the per-VMA move path exists and does not take the exclusive mmap lock. Use the existing address-ordered two-PTL helper for mremap PTE, PMD, PUD and huge-page forms. In the PTE case map both tables, take both locks, and revalidate both PMDs before moving entries. Retain the source tree's devmap checks, rmap locking, TLB flush and error unwinding. The lock helper is internal and was never exported. |
| [CVE-2026-99664](https://www.cve.org/CVERecord/?id=CVE-2026-99664) | David Hildenbrand (Arm); mainline `79cb51d538abc459f74e046d27797d36b133ce0e`, Linux 6.18 `da57b6bce646946c083a48df644d43bb9d011f5f` | Re-read and classify the PUD under its PTL before the huge-entry path. Retain the existing 6.12 follow context and missing-page behavior. No new huge-PUD feature is enabled; the actual handset reachability depends on mapping types. |
| [CVE-2026-99665](https://www.cve.org/CVERecord/?id=CVE-2026-99665) | Chris Mason; mainline `fe2817e3d5ea347214342c9a5cd8bcbff20c0574`, Linux 6.18 `0af947ae4d91af0c6e9573220a1298cd8302b6c3` | With THP enabled, preserve a PTE's dirty bit on the existing 6.12 single-PTE clear path. Mark the folio dirty before removing the rmap, using the return value of `ptep_get_and_clear()` rather than the earlier snapshot. |

These are adaptations to the exact source, not a wholesale 6.18 import or a
6.12 version bump. No configuration, exported signature, ABI reference,
firmware, signing policy, SuSFS setting or module-load policy has been changed.
Absence of a prototype change is not a substitute for the linked ABI checks.

## Security sources and decisions

- [Linux stable](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/):
  `v6.12.119` dereferences to `e2e4b92c2b2a3ae78423cb343951cb5d2241b405`;
  the 6.12.y branch pointed at the same commit during this check. The local
  .119 replay ledger records upstream and downstream SHAs; a different SHA
  by itself is not a missing fix. For example, the two .119 stable refs for
  CVE-2026-99709 and CVE-2026-80612 map to `fb92687f2310f` and
  `e25c41ab7ffaa` respectively.
- [Linux CNA data](https://git.kernel.org/pub/scm/linux/security/vulns.git/):
  `c09138f1238fbe53819a78f1b7e1609861af8431`. Screened the 656 changed records
  between the Nov 19 snapshot `3c73eff14` and this tip: 528 with named 6.12
  stable fixes, 98 with only newer introduction ranges, and 30 older/mixed
  cases without a 6.12.x fix entry needing additional applicability triage.
  These are changed records, not 656 newly discovered vulnerabilities.
  The 528 stable refs yielded 10 literal ancestors, 35 explicit .119
  replay matches and 483 normalized subject matches in local history.
  Subject matches are a screening signal, not independent binary proof.
  Among the latter cases, this target does not select KSM (CVE-2026-99701),
  CIFS (CVE-2026-99380/99381/99383), ksmbd (CVE-2026-99792), Arm SMMUv3
  (CVE-2026-99586), or the x86 Xen PFN-cache path in CVE-2026-99472.
- [November Android bulletin](https://source.android.com/docs/security/bulletin/2026/2026-11-01):
  screened the linked commits for all 31 kernel entries, using source
  ancestry, local equivalent history and actual implementation/config where
  the IDs differ. The C Binder fix is present (`1c8fffa12a81`), DMA-BUF accounting
  fixes are present (`009c5006c969`, `f7d26609785c`, `6642375b48be`,
  `dd1ca7920b1e`, `f118039ccd1a`), and FUSE passthrough uses
  `vma_set_file()` (`517040c192ce`). The local pKVM handler already
  keeps non-protected vCPUs always on at EL2, and carries the remaining
  power-state fix series. The epoll core has the reference/teardown protection
  from `d66547138334`; it retains its local file-pointer lifetime code rather
  than needing the alternate-branch scoped-cleanup syntax. The kernel TLS
  code fixes are present in source but that feature is disabled here.
- The currently retained stock `rfcomm.ko`, `bluetooth.ko` and `nfc.ko`
  are **separate deployment gates** for CVE-2026-53256, CVE-2026-89774 and
  CVE-2026-64011, respectively. Common source fixes exist, but ABI
  compatibility and a rebuilt source copy do not establish patch status
  of those stock binary modules. Userspace and proprietary Qualcomm firmware
  items are not addressed by this boot-only change.
- ACK comparison tips: `android16-6.12-lts`
  `58f2449a557a99270b15eb9841152be2ead76472`;
  `android16-6.12` `ff6be2559af23135abf743199d61a2d1ea23a957`. This is a
  scoped audit of the current baseline, not all historical or private CVEs.

## Droidspaces and SuSFS

- The current [Droidspaces release](https://github.com/ravindu644/Droidspaces-OSS/releases/tag/v7.2.0)
  is v7.2.0 (`cdadb9eb5d0f86aacffff60776333e6f6b73a30a`).
  Official main `77e63323c937ae69e260aa8432d6f664997312c1` adds app,
  runtime and documentation changes; the checker source is unchanged since
  v7.2.0 and there is no new mandatory kernel feature. The newer namespace-LKM
  documentation concerns third-party modules, not this kernel's native
  support. Updating the Android app/runtime separately is not a kernel patch.
- The built-in SuSFS is **v2.4.0**, pinned to
  `f83ae68add82d1c564e9428ba81dd537fa721924`. The official
  [Android 16 / 6.12 branch](https://gitlab.com/simonpunk/susfs4ksu/-/commits/gki-android16-6.12)
  still points to `9b0b13f069673b9b07b713a5c66977de4d62b175`. The three
  subsequent commits are KernelSU synchronization, a new per-UID hiding
  policy with matching userspace/supercall changes, and SELinux hook
  changes coupled to KernelSU. They are not a standalone version update
  or verified required maintenance for this KernelSU/ColorOS pairing.
  None was imported.

## Validation and deployment

- `git diff --check` and kernel checkpatch passed (zero errors or warnings).
  The extracted ordered-lock helper passed four host fixture cases for
  reversing argument order, aliasing one lock and releasing both locks.
  This is not a real concurrent kernel or lockdep test.
- Focused ARM64 compilation used the actual
  production compiler arguments and generated configuration for
  `gup.o`, `khugepaged.o`, `mremap.o`, `huge_memory.o` and
  `userfaultfd.o`. All passed and retained the same exported symbol
  names. `hugetlb.o` is not selected (`CONFIG_HUGETLBFS=n`); its
  corresponding portability hunk is not a tested runtime change for
  this firmware.
- The exact frozen B4 raw Image
  (`ec8275a423e80291be34e1622bcf8baedec3054571afb1f69f73b31bcb222e8e`)
  passed the official v7.2.0 checker (`requirements_met=1`) and an Alpine
  start/file/stop workload in a disposable ARM64 VM. This is baseline
  evidence, not a test of a new kernel image; optional MACVLAN, NAT66 and
  PID-limit support remain outside the qualified configuration.
- Full production build, strict KMI, stock-module ABI, corrected ColorOS
  STG and official Google GKI must be checked on the new outputs before
  packaging. The physical canary report records the subsequently completed
  build, ABI and device checks. Local evidence is under
  `out/coloros201-nov24-security.g3HCtv/`, including the build log, exact
  focused compilation results, Linux CNA and Android bulletin ledgers,
  and the baseline Droidspaces guest log. The common source diff against
  `e536034` has SHA256
  `768cb2f3f8276c9ef785ec7258ed8e9b97ee0742d15255d0f29e414aad7a652b`.
- This audit preceded the physical canary. The previous qualified boot
  and rollback images, primary worktree, CI worktree/stash and production
  refs were preserved. The canary report distinguishes the later boot-only
  flash from release qualification.

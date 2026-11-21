# ColorOS .201 / Linux 6.12.118 — OPlus power-model suspend investigation

**Conclusion: the stock OPlus callback reads the ordinary monotonic
clock while timekeeping is suspended.** Verified on both CPU-cluster
power-off and power-on inside system suspend-to-idle. The same trap and
ordering exist in the exact OEM rollback kernel. No new .118
timekeeping-path regression was found, and no safe boot-only source
correction is justified. Keep the
[conditionally qualified baseline](coloros17001201-warning-conditional-qualification.md);
extended suspend/power qualification is still open.

## Exact installed artifact and warning

CPH2747 ColorOS `17.0.1.201(EX01)`, slot A, source
`76b73bd515be1ed698c7df08ae29666eb3848b08`,
`6.12.118-android16-6-o-g76b73bd515be-4k`. Read-only wireless ADB
re-read the complete `boot_a` hash:
`8aecec4a35214a1ecb27e2170c28dcdd444542ac764bb3d84d3ab97447bfe16b`.
Exact stock rollback:
`e38045b1bdb0e51d71833206c848715cbc862f6fdc2039887cd7cd8248da5a4f`.
The existing 89-file evidence manifest reverified before and after
the offline work and was not edited.

The WARN header is
`WARNING: CPU: … PID: 0 at kernel/time/timekeeping.c:747 ktime_get+0xdc/0x118`.
The exact linked .118 vmlinux loads `timekeeping_suspended`, then
`cbnz w8` at `ktime_get+0x8` branches to `d4210000`
(`brk #0x800`) at `+0xdc`. That is the ARM64 instruction for
`WARN_ON(timekeeping_suspended)`, not a KCFI trap or an exception
reported by the hardware clocksource. After the warning the function
continues along its normal read path; a returned value from this
window is not contractually safe for residency accounting.

The caller LR is
`powermodel_cluster_suspend_handler+0x58/0x114 [oplus_powermodel]`.
The stack must be read together with the LR: the first stack
frame after `ktime_get` can be `cluster_power_cb [qcom_lpm]`,
not the OPlus function. Exact module relocations resolve the apparent
gap; the warning branch runs before `ktime_get` makes its normal
stack frame.

| Stock .201 module | Mounted module SHA256 (both OTA copies match) | Build ID |
|---|---|---|
| `vendor_dlkm/oplus_powermodel.ko` (vendor_boot copy dormant in the oracle) | `5c81283d314e3d1ce3f84e6cab1db9455269798838f48611483338b2ddac1f75` | `5cc0b04c6a7e609075a10c600f1ac8c0fb82f181` |
| `vendor_dlkm/qcom_lpm.ko` (vendor_boot copy is the recovery provider) | `6497d4afee8bfdc0968d7637221e82c68115c541db0686e59d1eba04da4efa6f` | `ef0c963703cb6dd994f051fd10ccf1844e340045` |

On initialization, `oplus_powermodel` writes its function pointer
to the `qcom_lpm` export `lpm_cluster_notify_c2_state`. That export
and the OPlus import both carry CRC `0x6d784498`; the Qualcomm
indirect-call check and the OPlus function-entry prefix match
`0xfea22452` (KCFI). Inside the OPlus callback:

- `.text+0x3f0c` = handler `+0x54` has `R_AARCH64_CALL26 ktime_get`;
  its return address is the exact `+0x58` seen in LR. This occurs
  after the CPU index check but **before** the enabled/ready checks.
  The enabled path uses the return value in the C2 accounting call,
  and stores a state/timestamp pair under the existing spinlock.
- The Qualcomm frames at `cluster_power_cb+0x1b0` (off)
  and `+0x8c` (on) are the **returns from the indirect OPlus call**
  at `.text+0x1324` and `.text+0x1200`, respectively.
  They are not the returns from Qualcomm's own
  `ktime_get` sites. The observed warning is not evidence of a
  second direct Qualcomm clock-read bug.
- The kernel export and each stock module's import for
  `ktime_get` agree at CRC `0xff9a6e5c`. This is a **call-context
  violation**, not a symbol, layout, or KCFI failure.

### Both entry and early return run with the clock frozen

The full recurring route is shown in logical call order; the OPlus
function is proved by the LR and the indirect call instructions:

```text
cpuidle_enter_s2idle → enter_s2idle_proper
  tick_freeze → last online CPU can call timekeeping_suspend
  psci_enter_s2idle_domain_idle_state → __psci_enter_domain_idle_state
    dev_pm_genpd_suspend → genpd_sync_power_off → raw_notifier_call_chain
      cluster_power_cb (off) → powermodel_cluster_suspend_handler → ktime_get WARN
    PSCI CPU suspend entry / return
    dev_pm_genpd_resume → genpd_sync_power_on → raw_notifier_call_chain
      cluster_power_cb (on)  → powermodel_cluster_suspend_handler → ktime_get WARN
  return from enter_s2idle callback → tick_unfreeze → timekeeping_resume
```

`tick_freeze()` suspends global timekeeping when all online CPUs
have entered the frozen region; the first unfreeze resumes it.
Thus some callbacks can run before freeze or after unfreeze and not
warn. In the offending traces the clock is still frozen. Cluster
power-on here is early CPU-domain resume *inside* the callback, not
a warning after the system resume has finished. Ordinary runtime
cpuidle uses the runtime-PM branch, not the observed genpd suspend/
resume branch. No normal-runtime or separate platform deep-suspend
instance was established. The selected system mode was `[s2idle] deep`;
the trace proves the software path, not the eventual hardware residency.

## Timestamps, wakeups and counters

Reviewed the retained boot/canary diagnostics and the previous
qualification sample. The native stock before-flash record establishes
the OEM identity, but the preserved boot logs are filtered snapshots
and do not contain a stock-kernel suspend trace with this warning.
The fresh read was the entire **available** kernel ring buffer, not
an unbounded boot history. Android's kernel log buffer did not supply
older entries, and pstore was empty. No reboot, trace enable, forced
idle, or synthetic wakeup was used.

The fresh captured ring slice spans printk times `6951.896677`–
`6963.222182` seconds. It has **117 WARN headers**, not 117
independent failures: 114 lie within observed suspend windows and
three lie at the left boundary of the ring. Of 35 entry/exit-paired
windows, 23 contain WARN headers and observed wake markers. Two
windows have explicit abort messages and no matching WARN. The
interleaved/incomplete per-CPU prints are retained, not counted as
clean unique stacks. Clean entry and resume examples occur at the
same printed time; use the stack and the call-return addresses, not
timestamp order alone, to distinguish them.

| Printk timestamp (s) | Evidence |
|---|---|
| 6952.826522 | `PM: suspend entry (s2idle)` |
| 6952.980646 | OPlus LR and both genpd call chains above |
| 6952.981226 | IRQ 485 reported as `gunyah_vcpu` |
| 6953.053760 | `PM: suspend exit` (15:25:04.674230115 UTC in this window) |
| 6955.773057–6955.862033 | A separate completed window logs failure/early-wake at 6955.830513, with zero matching WARN headers. |
| 6963.016886–6963.205854 | Separate failure/early-wake at 6963.162016 and a wake marker at 6963.174140, with zero matching WARN headers. |

The slice contains 19 Gunyah wake records and nine WLAN wake records
(for example `BPF_ALLOW`, denoting a WLAN wake filter outcome, not
the separate boot BPF/BHB warning). They do not show that the WARN
caused or prevented a particular wake or suspend success. The sampled
sysfs wakeup tallies were nonzero; they are aggregate, not reliable
per-warning attribution. The active and only advertised
clocksource was `arch_sys_counter`; no clocksource switch,
instability, clocksource watchdog, OPlus watchdog bark/bite, or
hung-task marker was found in the captured window. A watchdog
module name in the WARN's loaded-module list is not a watchdog event.
No claim is made about unretained system history.

Between 09:19:45 and 09:27:17 CST the suspend success counter advanced
`2744 → 3210`; aggregate failures/aborts `1646 → 1902`
(`failed_freeze 1266 → 1464`, `failed_suspend 332 → 389`).
`failed_resume` remained zero; the initial snapshot also had
zero early/noirq resume failures. At the first sample the last
recorded step was freeze, errno `-16`, with a separately retained
PCI device field. Do not assign the entire failure delta to that
device, or to the WARN; no per-attempt join exists for these counters.
The phone remained booted and user0 was `RUNNING_UNLOCKED`.

## Baseline and upstream comparisons

- Local qualified .117 `f8e4923a0bc5`, .117 LTS `eba5858c87e7`,
  and .118 `76b73bd515be` have **identical Git blobs** for
  `kernel/time/{timekeeping,tick-common}.c`,
  `drivers/cpuidle/{cpuidle,cpuidle-psci}.c`,
  `drivers/pmdomain/core.c`, and `kernel/power/suspend.c`
  under `kernel_platform/common`. All six also match the remotely
  checked [Google android16-6.12-lts tip `c5e3d3a3d3b60`](https://android.googlesource.com/kernel/common/+/c5e3d3a3d3b60eb95c36e3c49dbfe7a34c220c5a).
  The archived .102 OPlus module has a different whole-file hash and
  build ID, but the disassembly and relocations of this handler
  are identical to .201. This is source/binary comparison, not a
  negative or positive .117 run with the exact .201 contracts.
- Linux stable tags [v6.12.117](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tag/?h=v6.12.117)
  (`d3417611a3b4`) and [v6.12.118](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tag/?h=v6.12.118)
  (`b869a5313cbf`, also the remotely checked `linux-6.12.y` tip)
  match each other on those six files. No fix for this OPlus
  caller appeared in that stable interval. Upstream has historically
  treated reads from frozen-time genpd latency paths as invalid
  rather than relaxing the accessor; see the
  [genpd syscore timing patch](https://lkml.iu.edu/1505.3/04055.html).
- Exact .201 stock rollback kernel `6.12.69-android16-6-g5fd39602326e-ab16372235-4k`,
  raw Image SHA256
  `8a42827cc541b10c3c7f3e6774193ed9bed6a905e1bea4bae6f760c43c29bfc5`.
  A host-only analysis ELF was recovered with
  [vmlinux-to-elf 1.4.0](https://pypi.org/project/vmlinux-to-elf/1.4.0/);
  its symbols were checked against the machine code. The OEM
  `ktime_get` also loads the suspended flag and branches to the
  **same** `brk #0x800` at `+0xdc`; OEM `enter_s2idle_proper`
  calls `tick_freeze` before its indirect callback and
  `tick_unfreeze` after it. OEM `tick_freeze` calls
  `timekeeping_suspend`, and OEM PSCI has the same genpd call pair
  around firmware idle. Thus the invalid combination of the stock
  driver and an existing core diagnostic predates the .118
  implementation. Actual warning frequency on a running OEM kernel
  remains **unverified** without a qualified comparable stock log.
- The public OnePlus SM8850 source and the locally archived vendor
  trees did not supply the exact OPlus power-model implementation
  or private headers. Public Qualcomm `qcom-cluster-lpm.c` alone
  does not establish the exact OEM callback integration. No
  concrete firmware/clocksource counter regression was identified.

## Repair decision and external dependency

**No kernel patch attempted; no new boot candidate.** Do not remove
the diagnostic, change the behavior/CRC of `ktime_get`, unfreeze
timekeeping early, force a sleep mode, or reroute/disable the callback
globally. An early return based only on a runtime-enabled flag
would not correct reads made with accounting enabled.

The vendor needs to fix both halves of the OPlus callback: do not
read the ordinary monotonic clock while executing in the frozen
s2idle window. For a runtime-only C2 statistic, exclude the entire
s2idle measurement pair (including cleanup of any pending sample),
before touching the clock, while preserving normal idle accounting,
locking, notification and wake behavior. If system-suspend residency
is required, take matched snapshots at verified safe boundaries
outside the frozen window using a clock whose suspend semantics
match that statistic; handle aborted attempts and repeated cluster
transitions. The documented
[fast time accessors](https://www.kernel.org/doc/html/v6.12/core-api/timekeeping.html)
can be read in suspend but may freeze or jump. Swapping in
`ktime_get_mono_fast_ns()` with no accounting redesign is not a
validated fix; neither is switching to another normal accessor.

Obtain the matching .201 `oplus_powermodel` source/private headers,
C2/PMU state and locking contract, and matching Qualcomm LPM
integration/SDK or an OEM-fixed driver set. A subsequent replacement
must verify the callback KCFI ID/CRC, provider/importer chain,
normal/recovery owners, no lost pairs or bogus timestamp deltas,
and the authenticated package/rollback contract. A physical gate
should compare against an exact stock .201 trace, measure controlled
unplugged s2idle success/abort counts and wake sources, confirm normal
idle, test real and deliberately interrupted suspend in an approved
session, and verify clock continuity, battery, thermals and resume
function. Success means both correct accounting and no invalid reads,
not merely fewer log lines.

## Validation and final stability disposition

Fresh offline rerun on **unchanged** source `76b73bd515be`:

| Gate | Result |
|---|---|
| Common build, strict KMI symbol-list target, ABI generation | **PASS** (cached Kleaf actions); raw Image, config and both symvers tables match the frozen artifacts byte for byte. |
| Corrected independent ColorOS ABI extraction/comparison | **PASS / exit 0**; newly extracted filtered STG byte-identical to the qualified graph. |
| Full exact .201 module/provider oracle | **PASS**; 1,031 active compatible, 38 dormant; 60,835 hard imports; 61,910 CRC matches; zero missing providers, CRC, namespace, policy or protected-export conflicts. |
| Linked private layouts and KCFI | **PASS** in scope: 33 linked layouts, 2,534 shared C type IDs, seven linked prefixes; exact vendor callback match addressed above. No claim of exhaustive opaque vendor layouts. |
| ARM64 guest regression | **PASS**: namespaces, seccomp, cgroups, IPC, IPv4 NAT, ext4/F2FS remount, OverlayFS. QEMU virt does not exercise the physical Qualcomm/OPlus suspend callback. |
| Official enforced GKI | Preserved **FAIL / exit 4**, 76 documented blocks; not rerun for this unchanged baseline. |

New local receipts, timeline, parser, binary excerpts and rerun
outputs: `out/coloros17001201-powermodel-evidence/`. No module or
kernel code was changed. No new image was packaged, and nothing
was installed, flashed, rebooted, pushed, or published. Keep this
boot-only baseline conditionally qualified; **suspend-related
warning and full power/stability qualification remain open**.

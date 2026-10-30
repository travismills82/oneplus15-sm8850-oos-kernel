# Five-CVE isolated regression tests

These test-only files do not enter the production kernel or boot image.
They cover the CVE-2026-99103 workqueue repair, CVE-2026-99136 dm-crypt repair,
the CVE-2026-99138/99139 TCP pair and CVE-2026-99167 TC action repair.

Use only a disposable x86_64 QEMU/KVM guest with `-nic none`, no disk passed
through from the host, and the exact `five_cves_vm=1` command-line marker.
Never load the test module or run privileged DM/TC tests on the host or phone.
The additional VM Kconfig fragment is not production configuration.

## Build

From this worktree root:

```sh
bash tools/testing/five-cves/build-vm.sh /absolute/new-disposable-directory
```

Build the external `workqueue-test` against that generated kernel and sign it
with its generated test key through normal `scripts/sign-file` processing.
Do not copy the private test key into a source commit or release package.
The guest retains MODULE_SIG and MODVERSIONS. Create its initramfs using:

```sh
bash tools/testing/five-cves/make-initramfs.sh /absolute/new-disposable-directory
```

Only after inspecting inputs, boot the generated bzImage and initramfs with
KVM, 2 vCPUs, 3 GiB RAM, serial capture, no NIC, and no attached disks.
`qmp-finish.py` is an optional private-socket shutdown helper. It recognizes
both final PASS and FAIL markers; it does not manufacture test success.

## Evidence and limits

- Workqueue: bounded inactive-work cancellation/pool-attribute churn; per-CPU,
  ordered and empty controls. Cleanup releases blocked work on every path.
- TCP: concurrent Reno/Cubic switching and getsockopt reads, IPv4/IPv6 verified
  payloads, short/zero/full buffers and EFAULT cases. This does **not** exercise
  dynamic BPF struct_ops retirement.
- DM: tmpfs-backed loop mapping, AES-XTS direct-I/O roundtrip, lower-device
  errors requiring EIO, and restored data/cleanup. This is not a deterministic
  reproduction of the interrupt/status race or a test of Android FBE.
- TC: action binding-count lifecycle and concurrent filter/action/delete/dump
  operations inside a private guest network namespace.

Require all test markers and a complete serial log with no KASAN, BUG, Oops,
WARN, RCU, refcount, lockup or hung-task fault. Expected injected lower-device
I/O-error messages are not sanitizer failures. A final shell PASS alone is
insufficient without examining those diagnostics.

Finite stress cannot prove absence of every race. This x86_64 VM cannot
qualify ARM64, Qualcomm firmware, stock DLKM behavior, Android policy, FBE,
physical networking, radios or power management. Those require separate
static production gates and an explicitly authorized boot-only phone canary.

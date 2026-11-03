# Droidspaces validation helpers

These are diagnostic tools, not a phone installer or a stable-release gate by
themselves. Exact build, oracle, VM and device results are in
[`docs/features/droidspaces-coloros17.md`](../../../docs/features/droidspaces-coloros17.md).

- `test_source_contract.py`: narrow source/config/policy guards against the
  pinned pre-feature baseline. Does not prove runtime compatibility.
- `audit-config.py`: compare actual generated configs and enforce the reviewed
  dependency closure rather than trusting defconfig requests.
- `compare-compiled-abi.py`: compare existing export records, compiled BTF
  sizes/member offsets and shared KCFI IDs in the supplied matching artifacts.
- `verify-provider-policy.py`: inspect the real linked GKI import/export policy
  and vmlinux-first precedence against the complete original module oracle.
- `classify-gki.py`: retain enforced GKI FAIL and distinguish unchanged owners
  from the reviewed IPC-padding addition. Never updates the ABI reference.
- `probe-bindgen.py` / `probe-rust-bindings.py`: use trusted pinned Kbuild command
  records and toolchains for isolated generated-output diagnostics. Do not feed
  them an untrusted build directory or substitute unrelated SDKs.
- `run-vm.py`: boot the supplied ARM64 Image in disposable QEMU TCG without host
  block devices or a physical network interface. Record exact input hashes.
- `guest-init.sh` / `namespace-smoke.c`: **VM-only fixtures. Never run them on a
  phone or the host.** They deliberately manipulate guest mounts, namespaces,
  IPC and cgroups. The runner stops its own VM after the completion marker.
- `package-boot.py`: historical offline packager pinned to the pre-commit
  qualification checkout, exact rollback image and recorded evidence names.
  It intentionally refuses other HEADs and existing frozen destinations. It is
  not a generic repackager for arbitrary current commits. Published source
  corresponds to the tested common tree; do not rebuild tested bytes merely
  to replace the historical build-parent release suffix.

The actual Droidspaces checker is an independently hash-verified upstream
artifact or the owner's separately identified installed backend. It is not
vendored here. Its JSON `requirements_met` field, not exit code alone, is the
acceptance signal. Its transient network probes are not strictly read-only.

No helper grants permission to flash, change slots, modify DLKM/init_boot/vbmeta,
disable security enforcement, publish, or claim full distribution support.

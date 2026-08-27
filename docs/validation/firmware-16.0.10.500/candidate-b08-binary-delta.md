# Candidate B08 binary delta

## Accepted runtime boundary

Candidate B08 descends from physically qualified B07 commit
`c2629b3fd92bc913921be48c60dc4b1ad7c68b94`. Its single runtime commit is
`338c09465853ddbccde4861e14f8c8fa2f24342e`.

The complete tracked B07-to-B08 runtime diff is one file, eight insertions and
seven deletions, all in
`kernel_platform/common/net/netfilter/xt_quota2.c`. It changes only the failure
cleanup in the static `q2_get_counter()` helper. There is no config, UAPI,
header, DT, firmware, userspace, FBE/storage, system_dlkm, vendor_dlkm or
vendor_boot input delta.

## Generated-machine-code proof

The compiler inlines `q2_get_counter()` into `quota_mt2_check()`. Saved B07 and
B08 disassemblies and their full diff are:

```text
out/oos1610500-custom-r53-b08-final/b07-quota_mt2_check-full.disasm
out/oos1610500-custom-r53-b08-final/b08-quota_mt2_check-full.disasm
out/oos1610500-custom-r53-b08-final/quota_mt2_check-full.diff
```

The B08 code contains the expected atomic decrement/test on the counter
reference. The zero-reference branch unlinks and frees the counter; the
non-zero branch clears the procfs-entry field and retains the live object. The
new control flow is absent from B07. This is the selected source correction
emitted into Image.

## Fail-closed controls

| Contract | Result |
|---|---|
| tracked runtime paths outside `net/netfilter/xt_quota2.c` | 0 |
| semantic `.config` versus B07 | byte-identical |
| `Module.symvers` versus B07 | byte-identical |
| ABI reports | empty |
| strict KMI target | PASS |
| FBE/storage source delta | 0 |
| external header/shared-type delta | 0 |
| current-firmware module blockers | 0 across 1,020 modules |
| unexplained runtime groups | 0 |

Truthful SCM identity changes the kernel release token and normal linked
addresses/build metadata. The one-file source boundary, generated-code proof,
identical config/export contract, empty ABI reports and complete module audit
account for the functional delta.

Result: **PASS — B08 behavior is present and the binary boundary is closed.**

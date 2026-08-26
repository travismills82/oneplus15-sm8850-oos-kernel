# Candidate B07 binary delta

## Accepted runtime boundary

Candidate B07 descends from physically qualified B06 commit
`67ddf38649ea9b60fe30ec7be2a352620fe8ea2f`. Its single runtime commit is
`969639e8ca81ec5048338b4366cf17de28941029`.

The complete tracked B06-to-B07 runtime diff is one file, four insertions and
four deletions, all in `kernel_platform/common/net/ipv6/mcast.c`. It changes
only the local representation and use of the parsed multicast group address in
`__mld_query_work()`. There is no config, UAPI, header, DT, firmware,
userspace, FBE/storage, system_dlkm, vendor_dlkm, or vendor_boot input delta.

## Generated-machine-code proof

`__mld_query_work()` is inlined into the local `mld_query_work()` symbol. The
saved before/after disassemblies are:

```text
out/oos1610500-custom-r53-b07-final/b06-mld_query_work.disasm
out/oos1610500-custom-r53-b07-final/b07-mld_query_work.disasm
```

The B07 machine code has the expected larger stack frame and local 16-byte
address slot, initializes that slot before later pull/processing branches, and
uses the preserved local value in the final multicast-address comparison. This
is the source-level ownership correction emitted into Image.

## Fail-closed controls

| Contract | Result |
|---|---|
| tracked runtime paths outside `net/ipv6/mcast.c` | 0 |
| semantic `.config` versus B06 | byte-identical |
| `Module.symvers` versus B06 | byte-identical |
| ABI reports | empty |
| strict KMI target | PASS |
| FBE/storage source delta | 0 |
| external header/shared-type delta | 0 |
| current-firmware module blockers | 0 across 1,020 modules |
| unexplained runtime groups | 0 |

Truthful SCM identity changes the kernel release token and expected build
metadata. The source boundary, generated-code proof, identical config/export
contract, empty ABI reports, and full module audit account for the functional
delta.

Result: **PASS — B07 behavior is present and the binary boundary is closed.**

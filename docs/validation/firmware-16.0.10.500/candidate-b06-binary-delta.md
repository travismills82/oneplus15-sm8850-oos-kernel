# Candidate B06 binary delta

## Accepted runtime boundary

Candidate B06 descends from physically qualified B05 commit
`e17fd9021ded0d11c48ef5761df59ce931c6c0b5`. Its two runtime commits are:

- `42d15b0306b1b63097d5babe6e2c79cb9270670b` — SCC initialization;
- `aad7cdfe6542f3fb51d751236bbacde59e2d9b93` — MSG_PEEK/GC sequence guard.

The complete tracked B05-to-B06 runtime diff is three files, 63 insertions,
and 29 deletions, all under core AF_UNIX. There is no config, UAPI, DT,
firmware, userspace, system_dlkm, vendor_dlkm, or vendor_boot input delta.

## Generated-machine-code proof

B06 generated code contains:

- the increment and store of `unix_vertex_max_scc_index` in the
  `unix_add_edges` machine-code range;
- new `unix_peek_fpl` text (116 bytes), including the GC-in-progress tests,
  spin lock, seqcount updates, and store memory barrier;
- new internal `unix_scc_dead` text (372 bytes), which reads/retries the
  sequence counter before allowing SCC collection;
- calls from the AF_UNIX receive path corresponding to the new
  `unix_peek_fpl()` source call.

The saved before/after disassemblies are:

```text
out/oos1610500-custom-r53-b06-final/b05-af-unix-baseline.disasm
out/oos1610500-custom-r53-b06-final/b06-af-unix-hardening.disasm
```

The new local symbols are present in B06 `System.map`; they are absent from
B05 and absent from `Module.symvers`, proving that the generated behavior is
in vmlinux without creating an external provider export.

## Fail-closed controls

| Contract | Result |
|---|---|
| semantic `.config` versus B05 | byte-identical |
| `Module.symvers` versus B05 | byte-identical |
| ABI reports | empty |
| strict KMI | PASS |
| `struct unix_sock` pahole layout | byte-identical, 1,152 bytes |
| FBE/storage source delta | 0 |
| tracked runtime paths outside the three AF_UNIX files | 0 |
| current-firmware module blockers | 0 across 1,020 modules |

Truthful SCM identity and consequent layout/address metadata are expected.
The source boundary, machine-code proof, identical config/export contract,
empty ABI reports, and full retained-module audit leave zero unexplained
runtime groups.

Result: **PASS — B06 behavior is present and the binary boundary is closed.**

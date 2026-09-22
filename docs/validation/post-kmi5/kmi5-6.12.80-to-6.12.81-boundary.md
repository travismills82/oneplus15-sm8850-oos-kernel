# KMI5 first post-6.12.80 static boundary

## Result

The first dependency-complete Linux 6.12 stable subversion after the qualified
6.12.80 baseline is the first static-incompatible boundary:

| Boundary | Result |
|---|---|
| Qualified 6.12.80 (`0c510667a3f5`) | GOOD |
| Raw completed 6.12.81 (`5b2546f8d202`) | BAD |

The initial midpoint probes were also bad: raw 6.12.90 failed compilation on
the incompatible `fwnode_handle.flags` KABI replacement, and raw 6.12.85
failed GENDWARFKSYMS expansion on the removed `io_buffer_list` type. Testing
the adjacent 6.12.81 boundary then identified the first bad interval directly.

No device was flashed and no ABI reference or stock firmware payload was
modified.

## Raw 6.12.81 failure

The raw boundary failed the minimal common-kernel build before producing a
candidate `Module.symvers`:

```text
gendwarfksyms: __calculate_version: unknown type reference to
's#io_buffer_list' when expanding 's#io_kiocb'
```

The introducing stable commit is `cf9f60325cfa` (`io_uring/kbuf: switch to
storing struct io_buffer_list locally`, upstream `5fda51255439`, stable
cherry-pick provenance `1249eae601f9`). The newer runtime ownership model is
retained; Android commit `e01b3db50974` restores only the frozen KMI5 layout
and type anchors.

## Propagated networking break after the io_uring repair

With the io_uring repair alone, the enforced ABI report contained two direct
changed type roots:

- `struct net_device`: insertion of `mangleid_features` shifted 112 existing
  members by eight bytes.
- `struct nf_conntrack_expect`: insertion of the namespace field changed the
  size from 208 to 216 bytes and shifted 13 existing members.

Together those roots produced 1,204 CRC-only exported-symbol changes. The
frozen provider oracle measured 328 CRC-mismatching providers, 3,162 affected
retained import edges, and 402 affected stock consumers. There were no missing
providers.

## Compatibility implementation

The authoritative Android ABI reverts were preserved individually as evidence
and used to restore the qualified layout. The stable correctness behavior was
then reinstated through two downstream adaptations:

1. The TCPv4 GSO path retains `skb_header_pointer()` and the mangle-ID feature
   mask. The mask is stored in the unchanged `u64 __kabi_reserved8` member via
   typed internal accessors. The ABI record itself remains byte-for-byte
   represented as the original reserve.
2. `struct nf_conntrack_expect` remains 208 bytes. Namespace and optional zone
   state are stored in allocation-coupled XArray metadata. The entry is
   installed with the expectation, read under RCU, and erased from the
   expectation's RCU free callback. The helper lifetime and explicit-helper
   validation changes remain active.

## Final static evidence

Runtime-source commit: `0cb55e61f8a27bb9cd4a7d72d24bf9a99a2d7d36`.

| Gate | Result |
|---|---|
| Common kernel compile | PASS |
| Strict KMI symbol-list check | PASS |
| Enforced common ABI | PASS; empty report |
| KMI generation | 5 |
| Existing ABI additions/removals/signature changes | 0 / 0 / 0 |
| Existing CRC-only changes / changed type roots | 0 / 0 |
| Providers | 3,403 / 3,403 match |
| Retained provider edges | 44,149 / 44,149 match |
| Missing providers / CRC mismatches | 0 / 0 |
| Affected stock consumers | 0 |
| Stock module inventory | 1,020 rows; 984 active; 36 dormant |
| `struct nf_conntrack_expect` | 208 bytes; qualified layout retained |
| `struct net_device` | 2,688 bytes; raw `__kabi_reserved8` retained |

Final artifact hashes used by the static oracle:

```text
vmlinux:
c22b30d3a4b9505c531f4b19cbc3ca4e761c090945322352c010abbf035961ed

kernel_aarch64_Module.symvers:
1c0a6b40d7b7d4ad6acfb384f9ba64f6a7cddcc90a76aa173ad504633243e31f
```

The final `Module.symvers` hash is identical to the qualified 6.12.80 oracle.
The retained provider and module inventories are committed project oracles,
not a fresh extraction of proprietary `.400` module images; that evidence
boundary remains explicitly documented in `kmi5-6.12.80-abi-oracle.md`.

## Scope

This establishes a repaired static 6.12.81 boundary only. It does not qualify
a boot image, authorize an ABI-reference update, prove current proprietary
module signatures from re-extracted images, or authorize device flashing.

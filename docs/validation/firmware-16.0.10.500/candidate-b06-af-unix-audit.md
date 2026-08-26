# Candidate B06 AF_UNIX GC/SCC audit

## Selection

Candidate B06 is the complete two-commit AF_UNIX garbage-collection and SCC
hardening group. It is cumulative on the physically qualified B05 source and
does not include B07, an ACK point release, a configuration change, or any
DLKM-producing input.

The two fixes are ordered and treated as one coherent group:

| Order | Controlled provenance | ACK provenance | Upstream provenance | Purpose |
|---:|---|---|---|---|
| 1 | `95c71f7681904f8d1fd608d0347d488b4c9bbfd8` | `b952cc514e6f0e92951693f5131909bb1264ded3` | `60e6489f8e3b086bd1130ad4450a2c112e863791` | initialize new `unix_vertex.scc_index` values and retain the maximum live SCC index |
| 2 | `64b70eb984d0aa0bb1f2ca811a59eaf869b0061e` | `41c3d4eb44a78d5e5ec9dbcbaff1a383edd77212` | `e5b31d988a41549037b8d8721a3c3cae893d8670` | notify GC when `MSG_PEEK` duplicates SCM_RIGHTS references and defer an invalidated SCC decision |

The second controlled patch was authored against the tree that already
contained the SCC-initialization behavior. Both patches apply in that order to
B05 without semantic adaptation. Neither behavior exists in firmware-native
r53/B05: the old `unix_add_edge()` leaves reused `scc_index` state
uninitialized, and the old `unix_peek_fds()` has no GC sequence notification.

## Runtime delta

The complete B05-to-B06 runtime source boundary is:

| File | Changed functions/state | Result |
|---|---|---|
| `kernel_platform/common/net/unix/garbage.c` | `unix_add_edge`, `__unix_walk_scc`, `unix_walk_scc`, new `unix_peek_fpl`, new `unix_scc_dead`, fast SCC walk, GC state | fixes SCC identity reuse and GC/MSG_PEEK lifetime races |
| `kernel_platform/common/net/unix/af_unix.c` | `unix_peek_fds` | calls `unix_peek_fpl()` after duplicating an SCM_RIGHTS file list |
| `kernel_platform/common/include/net/af_unix.h` | internal `unix_peek_fpl` declaration | core-only declaration; no UAPI change |

The first fix is associated with CVE-2025-40214 in the controlled source
provenance. The second is an audited lifetime/race fix; no separate CVE is
asserted here.

## Structure and provider contract

- `struct unix_sock` remains 1,152 bytes and its complete pahole member layout
  is byte-identical between B05 and B06.
- `struct unix_vertex` already contains `scc_index`; B06 initializes and tracks
  that existing field but does not add, remove, reorder, or resize a member.
- no UAPI header changes;
- no Kconfig or device-tree changes;
- no exported-symbol addition, removal, signature change, or CRC change;
- `unix_peek_fpl` is a vmlinux-internal global with no `EXPORT_SYMBOL`;
- no vendor hook or external-module callback/table change;
- B05 and B06 `Module.symvers` are byte-identical.

Result: the complete AF_UNIX structural/provider boundary is kernel-internal.

## Android reachability

`CONFIG_UNIX=y`. Preserved B05 physical logs directly record Android init
creating many `/dev/socket` endpoints, including property service, logd, lmkd,
zygote/USAP, netd DNS/fwmark, adbd, tombstoned, traced, and Wi-Fi sockets. The
same logs contain active SELinux `unix_stream_socket` decisions. AF_UNIX is
therefore **CONFIRMED ACTIVE** on Canoe.

Those logs do not directly prove SCM_RIGHTS, cyclic file-descriptor graphs, or
the exact GC-versus-`MSG_PEEK` interleaving. Exact-fix coverage must remain a
separate physical-test field.

## Bounded physical reachability

A safe future harness can use fixed worker/process counts and an explicit
`RLIMIT_NOFILE` to cover:

- AF_UNIX stream, datagram, and seqpacket socketpairs;
- SCM_RIGHTS passing of duplicated eventfd, pipe, regular-file, and socket FDs;
- `recvmsg(MSG_PEEK)` followed by ordinary receive;
- small self-referential and two-/three-node cyclic FD graphs;
- concurrent `sendmsg`, `recvmsg`, `dup`, close, and process exit;
- repeated teardown sufficient to invoke normal AF_UNIX GC without exhausting
  memory or file tables.

High operation counts are adjacent-path evidence only. Unless existing traces
or deterministic harness evidence prove the vulnerable overlap, the correct
result is `EXACT FIX TRIGGER NOT OBSERVED`.

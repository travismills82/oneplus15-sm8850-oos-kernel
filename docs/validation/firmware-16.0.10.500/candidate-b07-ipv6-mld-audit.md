# Candidate B07: IPv6 MLD skb lifetime audit

## Selection

Candidate B07 selects one IPv6 multicast lifetime correction. The pre-fix
`__mld_query_work()` retained a pointer into the skb header, then called
`pskb_may_pull()` again. That pull can expand and relocate the skb head, so the
later group-address comparison could dereference stale storage. B07 copies the
16-byte multicast group address before any later pull and compares against the
copy.

This is a kernel-internal ownership correction, not a new feature. A later
physical PASS will establish firmware compatibility and stability only; it
will not itself establish a cybersecurity result.

## Provenance

| Layer | Commit |
|---|---|
| B07 runtime commit | `969639e8ca81ec5048338b4366cf17de28941029` |
| controlled project source | `c2bf320216a005451191c5c62520f137005f99fa` |
| Android Common Kernel | `4203806f700bb44ea0b05d484d9d40044b47fb04` |
| Linux 6.12 stable | `b2eb8886200b907fc71806869620609f0f4cacb0` |
| upstream origin | `791c91dc7a9dfb2457d5e29b8216a6484b9c4b40` |

The controlled patch is byte-for-byte equivalent to the selected source hunk.
The firmware-native r53/B06 implementation still held the pointer across the
later pull, and the controlled patch applied without adaptation. Therefore the
functional fix was not already present.

## Runtime reachability on the qualified B06 phone

Read-only inspection on 2026-08-26 established:

- `CONFIG_IPV6=y` in the exact B06 configuration;
- `/proc/net/igmp6` and `ip -6 maddr` contain active IPv6 multicast state;
- `wlan0` is up and contains all-nodes, solicited-node, and mDNS multicast
  memberships;
- `rmnet_data0`, `rmnet_data1`, `rmnet_data2`, and `rmnet_data3` contain IPv6
  multicast memberships, with global IPv6 addresses active on data interfaces;
- the normal path from ICMPv6 handling through `igmp6_event_query()` and the
  MLD workqueue is built into Image.

Classification:

- WLAN multicast membership machinery: **CONFIRMED ACTIVE**;
- cellular/RMNET multicast membership machinery: **CONFIRMED ACTIVE**;
- receipt of the exact MLD query followed by an skb-head-reallocating pull:
  **NOT OBSERVED**.

Membership presence proves meaningful adjacent runtime reachability, but does
not prove that the vulnerable query/reallocation interleaving occurred.

## Provider and module boundary

The only source path changed from qualified B06 is
`kernel_platform/common/net/ipv6/mcast.c`. The changed function is static and
is compiled into local `mld_query_work()` machine code. B07 changes no header,
UAPI, structure, enum, callback table, exported function, exported signature,
or module list.

The complete `Module.symvers` is byte-identical to B06. The full retained
current-firmware audit checked 1,020 module files and 57,216 import/CRC edges,
including WLAN, RMNET, IPA/GSI, USB networking and netfilter consumers, with
zero blocker. No stock module rebuild or re-sign is required.

## Other live ranking corrections

The same read-only audit found two paths more active than earlier evidence had
shown:

- OxygenOS installs named `quota2` rules for `rmnet_data2` and `globalAlert`;
- `wlan0` has an active SFQ child qdisc with a 10-second perturb interval.

They were reranked accordingly, but neither displaces B07: the MLD patch is a
single internal lifetime correction with no policy creation/error-path or
multi-fix coherence question. QTI GLINK UCSI is also live, but remains behind
kernel-internal fixes because it is a firmware-facing provider boundary.

## Targeted physical coverage

Safe future coverage can exercise IPv6 multicast joins/leaves on WLAN, bounded
multicast send/receive, Wi-Fi OFF/ON, cellular dual-stack recovery, handoff,
deep idle and full network regression. Do not manipulate carrier
infrastructure or manufacture memory pressure merely to force skb head
reallocation. If the exact path is not proven, the correct result is:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

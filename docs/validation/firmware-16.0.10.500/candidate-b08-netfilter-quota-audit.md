# Candidate B08 netfilter quota2 audit

## Selection result

**Selected: netfilter quota2 counter lifetime hardening.**

The B07 source builds `CONFIG_NETFILTER_XT_MATCH_QUOTA2=y`. Live OxygenOS
rules prove that the path is not merely compiled:

- IPv4 `bw_global_alert` had 1,588 packets and 697,318 bytes at capture;
- IPv6 `bw_global_alert` had 1,800,462 packets and 108,401,630 bytes;
- named `rmnet_data2` quota2 rules also exist, although their counters were
  zero in this snapshot.

This is **CONFIRMED ACTIVE WITH TRAFFIC** for the global accounting path.
Raw B07 evidence is retained at
`out/oos1610500-custom-r53-b08-audit/live-b07-quota2.txt`.

## Fix and provenance

The selected controlled commit is
`def8016a932815636cd777611d22be9078faf45e`, derived from ACK
`2993266692b6531d84c4459a60d4312d9e3b3b30` and upstream Linux
`4fa3c1db38c4d05772384cfad9b831b2bcb55d3b`.

In `q2_get_counter()`, another caller can acquire the newly listed counter
while `proc_create_data()` is in progress. If procfs creation then fails, the
old code unconditionally removed and freed the counter despite that acquired
reference. The fix decrements the creator reference under the list lock,
removes/frees only when it reaches zero, and otherwise clears the failed
`procfs_entry` pointer while retaining the live counter.

The patch changes one static function in
`kernel_platform/common/net/netfilter/xt_quota2.c`: eight insertions and seven
deletions. It is already absent from firmware-native r53/B07 and introduces no
config, UAPI, exported-symbol, shared-type, device-tree, firmware, FBE/storage,
or DLKM input change.

## Physical-test boundary

Normal WLAN and cellular traffic can verify that the actual quota2 rules,
counters, Android data accounting, netd, NetworkStack and handoff remain
compatible. Deliberately exhausting memory or forcing procfs allocation
failure is not an acceptable physical trigger. Unless natural evidence proves
the creation-failure race, coverage must remain:

`NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED`.

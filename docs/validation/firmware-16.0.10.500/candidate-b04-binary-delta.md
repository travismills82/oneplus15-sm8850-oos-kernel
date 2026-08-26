# Candidate B04 binary delta

## Accepted runtime boundary

Candidate B04 descends from qualified B03 freeze commit
`8f3ee08db58afff44881694676edd678d0baffe1`. Its runtime source commit is
`2f2631b951ced2ef05a4a9643610954b26736bcd`.

The complete tracked runtime diff is:

```text
kernel_platform/common/net/packet/af_packet.c | 1 insertion
```

No other common-kernel file, configuration, device-tree input, module source,
firmware contract, or userspace interface changes.

## Functional delta

In B03, `packet_release()` called `unregister_prot_hook(sk, false)` and then
immediately reset the cached device state while `po->num` remained nonzero.
B04 inserts:

```c
WRITE_ONCE(po->num, 0);
```

while `po->bind_lock` is held. This makes a racing `NETDEV_UP` notifier see the
socket as unbound and prevents it from relinking a closing fanout socket into
the fanout array.

## Configuration and provider contract

B03 and B04 `.config` hashes are identical:

`d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9`

B03 and B04 `Module.symvers` hashes are identical:

`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`

The change is in a local built-in function and alters no export, CRC,
structure shared with a module, or protected-provider surface. ABI reports
are empty, KMI checks pass, and the retained-module audit has zero blockers.

## Generated-machine-code proof

The B03 and B04 raw Images are both 39,889,408 bytes. At the same
`packet_release()` Image offset, B03 proceeds directly from the
`__unregister_prot_hook` call to the existing cached-device reset store:

```text
B03 bytes: da fb ff 97 7f c2 02 f9
                         ^ str xzr, [x19, #0x580]
```

B04 contains the new halfword store immediately after that call:

```text
B04 bytes: da fb ff 97 7f 6e 0a 79 7f c2 02 f9
                         ^ strh wzr, [x19, #0x536]
```

The `strh` is the compiled `WRITE_ONCE(po->num, 0)` for the 16-bit packet
protocol field. The following existing instruction is shifted by exactly one
AArch64 instruction. This deterministically proves that the B04 Image contains
the intended close-race guard. Other raw-byte differences include the truthful
SCM token and resulting code/debug layout changes; the one-file source
boundary, identical config and export CRCs, empty ABI reports, KMI pass, and
full module audit are the fail-closed controls for unexplained behavior.

| Artifact | B03 | B04 |
|---|---|---|
| Image SHA-256 | `8ba5dbb28e2688ed7a518358f14b10316d358b2e00001c28ef29f23e7ee8902a` | `674b906f1eb3989ccc7bb452f047f76a3d8fbfd0aa991394f1824663bae77888` |
| Boot SHA-256 | `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0` | `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3` |

Result: **PASS — generated machine code and runtime-input boundary match B04.**

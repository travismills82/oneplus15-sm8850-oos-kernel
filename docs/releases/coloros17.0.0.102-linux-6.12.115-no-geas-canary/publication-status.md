# ColorOS 17.0.0.102 .115 no-GEAS canary publication checkpoint

The owner authorized pushing the source history to main and requested a canary
release and TWRP ZIP. The subsequent explicit constraint is: **do not include
vbmeta**. No vbmeta asset or installer write is prepared under this checkpoint.
No release or installer is published while its safe deployment contract is
unresolved. This is not a stable release or a relaxation of the historical
controlled-stack release/installer gates.

## Exact frozen candidate

Kernel build parent: `a3b501fee3c3c9e44579f46fd78e2282d05e29e8`.
Kernel: `6.12.115-android16-6-o-ga3b501fee3c3-4k`.

| Artifact | SHA256 |
|---|---|
| boot.img | `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9` |
| vendor_dlkm.img | `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804` |

The exact payloads are preserved, not rebuilt. Source commits after the build
parent record packaging policy and validation without changing the compiled
kernel or GPU implementation.

## Why omitting the parent is not a generic install path

The older .114 stable package was boot-only and left stock DLKM/parent metadata
unchanged. The updated-GPU canary is a different architecture: its recorded
physical transaction installed boot B, modified vendor_dlkm and its matching
development-signed parent on B, with complete readbacks. No verification-disable,
rollback-index, key-enrollment or init_boot change was used.

The modified vendor_dlkm hashtree root is:

```text
b6ea40f9cd8abb4e1a84de5bfbd0dcecefe170321fa2027c1012ea0d6f7684b4
```

The original stock vendor root is:

```text
6d38272aa876b7a3eb84e35d0f7335d38f044e43c19c9bb419e30ca4ff638000
```

The accepted matching parent currently installed on the test phone has SHA256:

```text
54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c
```

These are actual frozen AVB descriptor/readback values. A vendor footer does
not replace the parent's authenticated descriptor. Unlocked/ORANGE operation
and mutable Magisk GREEN/locked properties do not remove this dm-verity root
matching requirement. The source push does not alter any physical partition.

An installer that leaves vbmeta untouched could safely proceed only after
verifying an already-installed matching parent and every other firmware,
slot, snapshot, capacity, module and backup prerequisite. It must reject an
ordinary untouched-stock parent rather than claim broad ColorOS compatibility.
Such an artifact is an existing-matched-installation-only package, not a way
for stock-firmware users to acquire the new GPU without matched authentication.

## Current qualification and remaining choice

See the [extended report](../../validation/firmware-17.0.0.102/coloros1700102-linux-612115-no-geas-extended-qualification.md).
Bounded GLES and actual Vulkan graphics rendering passed, as did the enabled
module oracle and the owner's listed hands-on tests. GEAS remains excluded.
Power/performance comparisons and unplugged suspend remain incomplete. GKI is
still FAIL, not overridden by the source push.

The publication decision must therefore distinguish:

1. A restricted canary for devices already carrying the exact matching parent;
   no vbmeta payload/write, no claim of stock-parent installability, and honest
   offline-only installer validation until separately authorized physical ZIP
   testing occurs.
2. Deferring the GPU/DLKM release while developing or authorizing another
   authenticated deployment model. No verification bypass is proposed.

No ZIP should silently omit this requirement. The installed slot-B canary,
qualified rollback images, .114 stable release, Magisk init_boot and all existing
source/security/KABI history remain unchanged by this checkpoint.

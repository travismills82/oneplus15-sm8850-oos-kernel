# Candidate B05 binary delta

## Accepted runtime boundary

Candidate B05 descends from the physically qualified B04 freeze commit
`e5896aba2186c2f47cfc5d45d9d1f26cbff943eb`. Its sole runtime commit is
`515d73a3d5f436bb3b67d36ef1be44fafd22e0ae`.

The complete tracked B04-to-B05 runtime diff is:

```text
kernel_platform/common/drivers/usb/gadget/udc/core.c | 28 insertions, 1 deletion
```

No other common-kernel file, configuration, device-tree input, module source,
firmware contract, or userspace interface changes.

## Functional delta

The pre-fix gadget and UDC devices have decoupled lifetimes. A gadget device
can outlive the `struct usb_udc` that owns the pointer used by gadget-driver
matching and release. B05:

1. stores the gadget's original `device.release` callback in the private
   `struct usb_udc`;
2. replaces it with `usb_gadget_release()`;
3. takes an additional UDC device reference in `usb_add_gadget()`;
4. releases the UDC reference before invoking the original gadget release;
5. restores the callback and drops the reference on the add-error path.

The change is the exact ACK patch, applies without adaptation, and does not
alter any public header, exported symbol, callback table shared with a module,
or module-visible structure.

## Configuration and provider contract

B04 and B05 `.config` files are byte-identical:

`d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9`

B04 and B05 `Module.symvers` files are byte-identical:

`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`

The full ABI reports are empty, strict KMI checks pass, and the independently
rerun retained-module audit has zero blockers across 1,020 modules and 57,216
import/CRC edges.

## Generated-machine-code proof

`usb_gadget_release` is absent from the B04 symbol table and present as a local
text symbol in B05 at `ffffffc080b686c4`. Its B05 machine code:

- loads the cached original release callback from the UDC;
- calls `put_device(&udc->dev)`;
- invokes the original release callback with the gadget device.

The B05 `usb_add_gadget` disassembly contains the additional `get_device()`
call and two `put_device()` paths required by the successful-release and
error-cleanup lifetimes. The saved disassemblies are:

```text
out/oos1610500-custom-r53-b05-final/b04-usb-gadget-range.disasm
out/oos1610500-custom-r53-b05-final/b05-usb-gadget-range.disasm
```

| Artifact | B04 | B05 |
|---|---|---|
| Image SHA-256 | `674b906f1eb3989ccc7bb452f047f76a3d8fbfd0aa991394f1824663bae77888` | `312cbbb85d4a757fb58f5310c565e5a2116f637fec2f88d63fd662e04d7999fc` |
| Boot SHA-256 | `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3` | `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8` |

Other raw byte differences can include the truthful SCM token and consequent
code/debug layout changes. The one-file source boundary, identical config and
export CRCs, empty ABI reports, KMI pass, disassembly proof, and complete
module audit are the fail-closed controls for unexplained runtime behavior.

Result: **PASS — generated machine code and runtime-input boundary match B05.**

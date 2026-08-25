# Candidate B03 binary delta

## Accepted runtime boundary

Candidate B03 descends from physically qualified B02 commit
`635709f3b14eaef8778abfbe92b8fbec3ed7e02e`. Its runtime source commit is
`c3b68584dbb4638abe27a69b7f421826625d4a53`.

The complete tracked runtime diff is one file:

```text
kernel_platform/common/fs/eventpoll.c | 84 insertions, 64 deletions
```

There are no other changed common-kernel files, no device-tree or config
change, and no external module source change.

## Functional delta

The ordered series changes `epi_fget()`, `ep_remove_file()`,
`ep_remove_epi()`, `ep_remove()`, `ep_clear_and_put()`,
`eventpoll_release_file()`, `ep_insert()`, and `do_epoll_ctl()`.

The important contracts are:

1. `ep_remove()` obtains a stable reference to the watched file before using
   it through removal and drops the reference with `fput()` afterward.
2. file release uses the dedicated `ep_remove_file()` and
   `ep_remove_epi()` sequence rather than racing the ordinary control path.
3. `struct eventpoll` carries an RCU head and final release uses
   `kfree_rcu()` semantics so `ep_get_upwards_depth_proc()` readers cannot
   observe freed storage.

## Configuration and provider contract

B02 and B03 `.config` are byte-identical:

`d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9`

B02 and B03 `Module.symvers` are byte-identical:

`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`

Therefore B03 adds or removes no exported symbol and changes no exported CRC.
The ABI diff reports are empty and retained-module validation reports zero
provider-contract failures.

## Machine-code proof

The final B03 `vmlinux` contains the new local `ep_remove_epi` function at
`0xffffffc0804aa1c8`. `eventpoll_release_file()` calls it after unlinking the
file-side item, while the ordinary `ep_remove()` path calls it at
`0xffffffc0804aaf6c` and then calls `fput()` at
`0xffffffc0804aafe0` after the pinned reference is no longer needed.

The final release path in `ep_clear_and_put()` changed deterministically:

```text
B02: mov x0, x19; bl kfree
B03: add x0, x19, #0xc0; mov x1, x19; bl kvfree_call_rcu
```

The `#0xc0` address is the newly added `struct eventpoll::rcu` member. This
proves the generated binary contains the RCU-deferred final free rather than
the B02 immediate `kfree()` behavior.

The raw B02 and B03 Images retain the same 39,889,408-byte size. Their binary
containers differ beyond eventpoll because the truthful SCM release token
changes from B02 to B03 and is embedded in kernel version, modinfo, and debug
data; the code-size change also shifts later addresses. The source boundary,
byte-identical config and Module.symvers, empty ABI reports, KMI pass, and full
retained-module audit are the fail-closed controls for unexplained changes.

## Final artifact identities

| Artifact | SHA-256 |
|---|---|
| Image | `8ba5dbb28e2688ed7a518358f14b10316d358b2e00001c28ef29f23e7ee8902a` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `38b5f8934ee3efdc221c24542f7c93ae9c39f4909f9904e5d91c456a348b6ff4` |
| vmlinux | `be39b2001d6c74c3f5468a619487f39bb18d58ea2d52cf1fdda5ce6951a47a4e` |
| boot | `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0` |

Result: **PASS — generated machine code and runtime-input boundary match B03.**

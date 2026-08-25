# Candidate B02 binary delta

## Accepted runtime boundary

Candidate B02 descends from physically qualified B01 commit
`b9b15f8a22a906786729cf830c47d0c6cd237e9a`. Its runtime source commit is
`ab336ec00b4bf6a86fde5ba682852fefa06de0c8`.

The complete tracked runtime diff is one line in
`kernel_platform/common/kernel/bpf/hashtab.c`, inside `pcpu_init_value()`:

```text
copy_map_value_long(...) -> copy_map_value(...)
```

There are no other changed files under `kernel_platform/common`, no device-tree
change, no configuration change, and no external module source change.

## Source and functional provenance

The selected controlled-project source is commit
`13fd1f13482d4232def6618da916c2a1ee686f3a`. It is a direct backport of Android
Common Kernel commit `e0378419b0e20178b5d100b27c9cc7e51064202e`, whose upstream
Linux commit is `576afddfee8d1108ee299bf10f581593540d1a36`.

The firmware-native r53 implementation is pre-fix. The old helper rounds the
copy length to eight bytes. That can read beyond a source whose declared value
size is not eight-byte aligned, including a four-byte `CGROUP_STORAGE` value
used to update a per-CPU map. The new helper copies the declared map value size.

The path is compiled and reachable in the Canoe configuration:

```text
CONFIG_BPF=y
CONFIG_BPF_SYSCALL=y
CONFIG_CGROUP_BPF=y
CONFIG_BPF_EVENTS=y
```

## Configuration and provider contract

The B01 and B02 `.config` files are byte-identical:

`d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9`

The B01 and B02 `Module.symvers` files are also byte-identical:

`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`

The first build invocation exposed a workspace-default transition to
uncompressed DWARF. That output was rejected. The accepted B02 Image and all
ABI/KMI targets were rebuilt with B01's exact
`--//build/kernel/kleaf:zstd_dwarf_compression` setting.

## Machine-code proof

The final `vmlinux` contains `pcpu_init_value` at `0xffffffc0802d5114`.
Disassembly of the changed branch shows:

```text
ffffffc0802d51fc  mov x0, x24
ffffffc0802d5200  mov x1, x23
ffffffc0802d5204  mov x2, x20
ffffffc0802d5208  mov w3, w22
ffffffc0802d520c  mov w4, wzr
ffffffc0802d5210  bl  0xffffffc0802d4700 <bpf_obj_memcpy>
```

`w22` is the map's declared `value_size`; the call therefore implements the
size-aware `copy_map_value()` path rather than the old rounded long-copy path.
The all-other-CPU zero/preallocated handling remains unchanged.

The raw B01 and B02 Images have the same 39,889,408-byte size. Their byte-level
containers differ beyond this function because the truthful SCM release token
changes from B01 to B02 and is embedded in kernel version/modinfo/debug data;
the code change also shifts addresses and compressed/debug sections. The
fail-closed source, config, export, ABI/KMI, and retained-module checks are the
authoritative unexplained-delta controls. They report exactly one intended
runtime source group and zero unexplained runtime groups.

## Final artifact identities

| Artifact | SHA-256 |
|---|---|
| Image | `098f4c2e0ca1b27ec0e238919aa3080b116b3686f73a751cf9e1a39634b035e5` |
| `.config` | `d1462645149bbd78acf72356d7a971c68c9a385b250a90384f2309c8597699f9` |
| Module.symvers | `6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27` |
| System.map | `ee46b9564435c842bc850c97359bc99388fb0afcae45785d6f67a5b3ec31bfc5` |
| vmlinux | `ecc7d33e1ebe8861ee9ce78259895e34f4cd0cd023485b1ab3ed6ce690c56269` |

Result: **PASS — machine-code behavior and runtime-input boundary match B02.**

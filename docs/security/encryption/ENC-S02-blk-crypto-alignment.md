# ENC-S02: blk-crypto invalid-alignment status

## Scope

ENC-S02 changes only the status returned when blk-crypto rejects an
unaligned encrypted bio.  Valid encrypted I/O, key programming, crypto
profiles, data-unit numbers, fscrypt policies, and disk format are unchanged.
The kernel remains Linux 6.12.35 and Android KMI generation 5.

## Provenance

| Role | Commit | Subject |
|---|---|---|
| Upstream Linux fix | `0b39ca457241aeca07a613002512573e8804f93a` | `blk-crypto: use BLK_STS_INVAL for alignment errors` |
| Android Common 6.12 backport | `1f0f07fd8f4130852022297aa5e68df6780ccc25` | same subject |
| Local provenance-preserving backport | `5344dadb698bd519b76ed08120716c712c7bdaa2` | same subject and original Carlos Llamas authorship |

The implementation changes `BLK_STS_IOERR` to `BLK_STS_INVAL` for the two
existing alignment rejection paths in `blk_crypto_bio_prep()`.  This gives
callers an accurate invalid-argument result without changing successful I/O.

## Compatibility assessment

- On-disk or encryption format: unchanged.
- FBE key lifetime or hardware-wrapped-key behavior: unchanged.
- Valid bio/request construction: unchanged.
- ABI/KMI declarations and provider CRCs: unchanged.
- Kconfig: unchanged.
- Userspace tools impact: none.
- Reformat or key regeneration: not required.
- Stock supporting partitions: unchanged.

## Static validation

The following commands passed from `kernel_platform/` on source
`5344dadb698bd519b76ed08120716c712c7bdaa2`:

```text
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel build \
  //common:kernel_aarch64_abi \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //common:kernel_aarch64_abi_diff
tools/bazel run //common:kernel_aarch64_abi_dist -- \
  --destdir=out/enc-s02-abi-dist
tools/bazel build --kbuild_symtypes //common:kernel_aarch64
tools/bazel build //soc-repo:canoe_perf_abi
```

| Gate | Result |
|---|---|
| Canoe distribution | PASS |
| Enforced common ABI | PASS; empty diff |
| Strict KMI symbol list | PASS |
| Symtypes | PASS |
| Canoe device-module-inclusive ABI dump | PASS |
| ABI reference update | NONE |
| Configuration change | NONE |

The generated configuration is byte-identical to the physically tested
Phase 1 and ENC-S01 configuration, SHA-256
`166e1db5f0a5e5e33b801413d3f5255f8f92dcf8e210317d24d27cb3389897d4`.
The generated `Module.symvers` is also byte-identical to the qualified
provider oracle: 9,356 records, 575,272 bytes, SHA-256
`6889e56eb427705a9002c7b030b2af7b9a6153c874efb1cda5ddaf56be60aa27`.
This replays the accepted 1,020-module, 57,216-edge stock contract through
exact provider identity; the removed stock-binary extraction roots prevent a
fresh binary rescan in this worktree.

All 103 installed GKI modules are signed with SHA-512 by the expected build
key.  No host OpenSSL fallback, SHA-1 fallback, ABI suppression, KMI change,
or module-security change was made.

## Physical gate

ENC-S02 will be physically tested as a small cumulative boot-only canary with
ENC-S03 after that independent device-mapper invalid-key fix passes the same
static gates.  No device partition was written for ENC-S02 alone.

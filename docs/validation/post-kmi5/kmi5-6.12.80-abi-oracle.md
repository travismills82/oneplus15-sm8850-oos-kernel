# Qualified Linux 6.12.80 KMI5 ABI oracle

## Identity

This is the immutable source/static comparison baseline for the post-6.12.80
KMI5 boundary search. It is the exact source and production build used by the
physically accepted OxygenOS 16.0.9.400 boot canary.

| Field | Value |
|---|---|
| Source-producing commit | `0c510667a3f5e0da2e814d257e073bb9448f4b93` |
| Physical-report commit | `568c3db93d1ecf9fa26313bd32f8234e8f64ffa6` |
| Kernel | `6.12.80-android16-5-o-g0c510667a3f5-4k` |
| KMI generation | `5` |
| Toolchain | Clang `r536225`; Rust `1.82.0` |
| Tested firmware | `CPH2749_16.0.9.400(EX01)` |
| Tested boot SHA-256 | `3f0110f97f0c35fb6739f19440f62d1a2f83d5c117550532edba834d5d93d436` |

## Immutable build hashes

| Artifact | SHA-256 |
|---|---|
| Production `vmlinux` | `7b221ff757fac0ac435b2a7923e06aafbf2bbcee5be3c881d6f3d0a9da916ded` |
| Production `kernel_aarch64_Module.symvers` | `1c0a6b40d7b7d4ad6acfb384f9ba64f6a7cddcc90a76aa173ad504633243e31f` |
| Final Canoe `.config` | `9521839ee3f4b73bcd26ef98e382e8338d3bf3926df615df5261c71c2e0f64ab` |
| `gki/aarch64/abi.stg` | `404e7c64a602946faca79acbdf3c656295f33012bdb66030f3370284f25d191c` |

The enforced common ABI report for this exact candidate is empty. Replaying
the provider oracle against the production `Module.symvers` yields 3,403 of
3,403 matching providers, 44,149 of 44,149 retained provider edges, no missing
providers, no CRC mismatches, and no affected stock consumers.

## Retained stock-module oracles

| Oracle | Rows | SHA-256 |
|---|---:|---|
| `docs/validation/ack-modernization/ack-6.12.24-vendor-kmi-contract.tsv` | 3,403 providers | `9a6eb99973dcaf160a90369b0259c4b56d42842ac1b133e8ec7e93b792b5d300` |
| `docs/validation/firmware-16.0.10.500/ack-6.12.24-stock-module-compatibility.tsv` | 1,020 modules | `62afa8f11cc49e42a92ae4211688d8e47f51a2eaf3fa55e440bd1d8b26266148` |

The retained module inventory contains 984 active and 36 dormant rows. These
committed provider/import oracles are the project's preserved binary-module
evidence and are not a new scan of currently extracted `.400` images; the
original proprietary module extraction trees were removed during disk-space
cleanup. Boundary reports must retain that distinction.

## Use

`tools/kmi5-stock-provider-oracle.sh` compares a candidate production
`vmlinux` and `Module.symvers` with the retained provider/module population and
the enforced common ABI report for the same source. It fails closed on any
missing or CRC-mismatching stock provider, existing ABI removal, direct
signature change, CRC-only existing-symbol change, or changed type root.
Additions are inventoried separately and do not by themselves fail the oracle.

This oracle authorizes no ABI-reference update, image packaging, or device
write.

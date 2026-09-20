# Superseded Linux 6.12.110 first-test strategy

This plan was superseded when the repository owner selected Linux 6.12.80 as
the first post-6.12.52 physical checkpoint.  See
`linux-6.12.80-first-test-strategy.md`.  Linux 6.12.81 and later remain out of
scope until the 6.12.80 result is known.

The repository owner's selected first test target is Linux 6.12.110 while
retaining the Android 16 generation-5 KMI contract.  The intermediate
milestones remain source and static-review checkpoints; they are not build or
physical-test targets unless the final candidate fails and interval isolation
is required.

The source history must still contain every applicable non-merge Linux stable
commit individually and in dependency order.  The complete 6.12.53 to
6.12.110 interval contains 12,826 commits and is inventoried in
`oos1610500-linux-6.12.53-to-6.12.110-commits.tsv`.  This strategy does not
authorize squashing, importing the Android KMI generation-6 thaw, weakening
ABI/KMI checks, changing the ABI reference, replacing stock firmware
partitions, flashing, publishing, or updating `main`.

The first boot artifact may be produced only after the final 6.12.110 source
passes KMI5 configuration review, enforced common ABI, strict KMI, symtypes,
Canoe ABI and distribution builds, stock-module contract validation, private
ABI review, SHA-512 module-signing validation, FBE/storage review, and boot
container verification.  Any additions-only ABI reference update still
requires separate explicit authorization.

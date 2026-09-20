# ACK 6.12.40 to 6.12.52 linear history reconciliation

On 2026-09-19 the published ACK 6.12.41 through 6.12.52 history was
reconstructed as a linear monorepo first-parent sequence.  The earlier source
integration preserved the complete official history as the second parent of a
subtree merge, but GitHub's normal `main` history view collapsed that ancestry
behind one merge row.  This reconciliation changes history presentation and
commit identity only; it does not change the resolved kernel or build-input
content.

## Safety refs

The previous public `main` tip is retained permanently at:

- branch `archive/main-pre-linear-ack-6.12.52`;
- annotated tag `pre-linear-ack-6.12.52-20260919-2253`;
- commit `fa9fe14314c7b043f7d16f4b34397ff7d6eef68b`.

## Linear replay

- Downstream base: `c292946ff27bce535e7b8a0bc8a471026c81a32d`.
- Official ACK start: `c026dd5eaf39a29cfac9ed1405778d420ec79939`.
- Official ACK target: `36e5f6313583daf75ec31104ee8a186798a3b95f`.
- Replayed official commits: 2,298.
- Official non-merge commits: 2,255.
- Official merge records retained linearly: 43.
- First-parent commits in the replay: 2,298.
- Original author, author email, author date, and subject mismatches: 0.

Each non-merge commit records its authoritative source SHA in the standard
`cherry picked from commit` trailer.  Each flattened upstream merge record
contains an `Upstream-commit` trailer.  Original upstream authorship is
preserved; the reconstructed commits use `Travis Mills
<travismills82@gmail.com>` as committer.

The visible Linux stable boundary commits on the new first-parent line are:

| Version | Linear monorepo commit | Official source commit |
|---|---|---|
| 6.12.41 | `682ec697ff51` | `8f5ff9784f32` |
| 6.12.42 | `0e4d5bbe0576` | `880e4ff5d6c8` |
| 6.12.43 | `c6a826c6bdd4` | `9becd7c25c61` |
| 6.12.44 | `687ed55b4fc9` | `11a24528d080` |
| 6.12.45 | `14adb0b5fb25` | `b0c51e95f54e` |
| 6.12.46 | `4ddd046db51c` | `d497f0738df9` |
| 6.12.47 | `9905a6d6a9c7` | `f6cf124428f5` |
| 6.12.48 | `bfbe4aa3ec97` | `f1e375d5eb68` |
| 6.12.49 | `670295c927df` | `da274362a7bd` |
| 6.12.50 | `6f00832ae0e5` | `72b82d56b821` |
| 6.12.51 | `9c642c7ec375` | `a9152eb181ad` |
| 6.12.52 | `3ac71ad887cb` | `2b2cbdcede38` |

## Tree and identity proof

After the official replay, commit `7d8c97df7cfa` reapplies the 37 reviewed
OnePlus/OxygenOS conflict resolutions from the original subtree merge.  Its
tree is exactly the original merge tree `5280e838c3757b2ac9be4a07f6c1d2b49be57909`.
The manifest, handoff documentation, OOS compatibility, KABI, and ABI commits
were then replayed separately.

Before adding this report, the reconstructed tip tree was exactly
`53ebeb0e0a9e892d93e2d9d80aac4e0c4962bd05`, identical to the previous public
`main` tree.  No `Codex <codex@openai.com>` author or committer record remains
in the rewritten segment.  No build was repeated because the kernel and build
input bytes are unchanged; prior build or physical-test claims are neither
expanded nor reclassified by this history-only operation.

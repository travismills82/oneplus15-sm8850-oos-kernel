# Controlled-stack release process

## State machine

Every controlled release advances through this fail-closed sequence:

```text
DEVELOPMENT
  -> STATIC_VALIDATED
  -> PHYSICAL_TEST_REQUIRED
  -> PHYSICAL_PASS
  -> BASELINE_FROZEN
  -> RELEASE_PREFLIGHT_PASS
  -> PHYSICALLY_TESTED_ZIP
  -> PACKAGE_VERIFIED
  -> RELEASE_READY
  -> PUBLISHED
```

No state may be skipped. A failure returns the work to the immediately prior
physically qualified baseline; it never broadens the candidate automatically.

## Required records

Each release owns a directory under `docs/releases/<release-id>/` containing:

- `release-manifest.json`: device, firmware, kernel contract, immutable
  payload hashes, ZIP identity, write boundary, source identity, and tag
  target.
- `validation-signoff.json`: static, TWRP, read-back, boot, subsystem, load,
  and error-scan results, including non-blocking observations.
- `release-notes.md`: installation, qualified behavior, coverage boundaries,
  and integrity values.

The committed manifest uses `TAG_TARGET` for the self-referential release
commit. After the one release-engineering commit exists,
`finalize-controlled-stack-release-manifest.py` replaces it in the release
asset with the exact 40-character tag-target commit. The committed tag ref and
published manifest are both verified after publication.

## Preparation

1. Build the candidate and pass its subsystem-specific static gates.
2. Freeze the exact `boot`, `system_dlkm`, and `vendor_dlkm` hashes.
3. Physically test those exact payloads.
4. Package those exact files with
   `package-controlled-oos-stack.sh --profile controlled-v1-modernized-twrp`.
5. Run `validate-controlled-stack-twrp-zip.py`, AVB verification, filesystem
   checks, and a second deterministic build. Require a byte-identical ZIP.
6. Install the exact final ZIP through TWRP, verify backups and partition
   read-backs, boot Android, and complete subsystem regression testing.
7. Freeze the ZIP SHA-256 and fill in the machine-readable signoff. Never
   rebuild a payload or silently substitute a reconstructed ZIP at this stage.

## Release preflight

Run the release verifier first against committed metadata, then against the
physically tested ZIP:

```text
python3 tools/verify-controlled-stack-release.py \
  --manifest docs/releases/<release-id>/release-manifest.json \
  --signoff docs/releases/<release-id>/validation-signoff.json \
  --allow-symbolic-release-commit

python3 tools/verify-controlled-stack-release.py \
  --manifest <finalized-release-manifest.json> \
  --signoff docs/releases/<release-id>/validation-signoff.json \
  --zip <physically-tested.zip> \
  --sha256sums <SHA256SUMS> \
  --release-commit <release-commit>
```

The verifier rejects an incorrect device, firmware, kernel, payload, helper,
write boundary, ZIP hash, live signoff, load contract, release commit, or
release-only path set.

## Publication

1. Verify the remote tag and GitHub Release do not already exist.
2. Push the single release-engineering commit without rewriting history.
3. Create and push an annotated tag at that commit.
4. Publish the physically tested ZIP, `SHA256SUMS`, finalized
   `release-manifest.json`, and `validation-signoff.json`.
5. Download all published assets into a clean directory.
6. Re-run the verifier and require the downloaded ZIP SHA-256 to match the
   physically tested ZIP exactly.
7. Confirm the tag commit, GitHub Release tag, tested source identity, and
   payload hashes before declaring `PUBLISHED`.

Private signing material, signing repositories, partition backups, live
captures, and device data are never release assets.

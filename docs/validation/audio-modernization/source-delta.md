# Audio .046 to .059 source/value audit

## Provenance

The current CPH2747 source is synchronization commit
`5ab2a689ff87d7d28c511f1762cf41c1b90d965a`, which explicitly records
`AU_TECHPACK_AUDIO_HANDSET.LA.11.0.R1.00.00.00.000.046` for both
`audio-kernel-ar` and `audio-devicetree`.  The comparison source is official
OnePlus same-SM8850 synchronization commit
`d447f713d6403f707a2910383495f4ada98cfa4d`, which explicitly records `.059`.
The latter snapshot is for PLZ110, not CPH2747, so its board data is evidence
for source comparison and is not an authority for Canoe device configuration.

The synchronized delta contains 57 paths: 19 C/H runtime-source paths, seven
build paths, 29 device-tree/binding paths, and two other paths.  Canoe keeps its
existing Infiniti/CPH2747 audio overlay; the `.059` snapshot deletes or renames
that overlay and adds foreign Fairlady/Macan data.

## Runtime-value classification

| Area | Runtime change | Canoe applicability | Decision |
|---|---|---|---|
| GPR transport | `gpr_remove()` synchronously cancels `notifier_reg_work` before deregistration | Active GPR/RPMSG transport; no public-header, DT, firmware, or HAL change | Candidate: teardown work-race/lifetime hardening |
| LPASS TX macro | Adds per-decimator reference accounting and changes a tuning constant | Active microphone provider, but central shared capture behavior | Real value, broader provider risk; retain `.046` for first experiment |
| SoundWire master | Improves wake-IRQ error handling and underflow feedback throttling | Active central provider for codecs, BT, and haptics | Real value, broad consumer/power closure; defer |
| WCD939x | Adds ECID/efuse information and probe-failure supply cleanup | Active WCD9395 path | Mostly diagnostics and failed-probe cleanup; lower normal-runtime value |
| TFA98xx | Adds alternate firmware-name/register selection and changes feedback initialization | Active speakers use TFA986x, but current OOS supplies the existing `tfa98xx.cnt` contract | Defer: new DT/firmware selection is not justified for CPH2747 |
| AW882xx/SIPA | Vendor-specific fallback/reset changes | Modules may be packaged, but CPH2747 runtime speakers are NXP TFA986x | Not active hardware path |
| WSA885x | Adds a new I2C amplifier driver | Not the CPH2747 speaker path | Foreign hardware; drop |
| machine/MSM DAI | Adds routes and interfaces associated with newer/foreign configuration | Existing Canoe machine and userspace contract is qualified | Device-specific; drop |

## Selected change

Only the functional `.059` line below is imported for the first candidate:

```c
cancel_work_sync(&gpr_priv->notifier_reg_work);
```

`gpr_probe()` initializes and can schedule this work.  The `.046` remove path
could proceed through notifier and device teardown while the queued callback
was still pending.  `.059` waits for the callback before teardown.  This is
described conservatively as teardown work-race/lifetime hardening; the source
comparison alone does not prove a security issue or a complete UAF fix.

The GPR public headers, exported API set, module parameters, DT binding, DSP
firmware contract, and OxygenOS audio userspace contract are unchanged by this
candidate.  Runtime consumers are `audio_pkt_dlkm`, `audio_prm_dlkm`, and
`spf_core_dlkm`; any required changes to them are exact-stock pre-signature
payload re-signing for the controlled protected-export trust contract, not
source upgrades.

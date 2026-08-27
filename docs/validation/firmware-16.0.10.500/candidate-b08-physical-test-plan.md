# Candidate B08 physical test plan

Candidate B08 changes only `q2_get_counter()` in the built-in quota2 match.
The exact candidate boot hash recorded by the static manifest is the only
permitted write, and only `boot_a` may be changed. The stock current-firmware
system_dlkm, vendor_dlkm, vendor_boot, VBMeta and slot metadata remain untouched.

## Preflight and boot gates

1. Confirm the phone is healthy on qualified B07, slot `_a`, boot-complete and
   `RUNNING_UNLOCKED` user0.
2. Verify the candidate hash without rebuilding it; back up `boot_a`, verify
   source/backup equality, perform the hardened recovery dry run, flash boot
   only, and require exact full-partition read-back.
3. Require two clean Android boots, unlocked existing user0 on both, no
   `init_user0_failed`, no Rescue Party and the expected B08 kernel release.

## quota2-targeted coverage

Capture IPv4 and IPv6 `iptables-save -c` output before, during and after the
test. Record the named `globalAlert` and active-interface quota2 rules and
their packet/byte counters.

Exercise only normal Android policy and traffic paths:

- sustained WLAN traffic with concurrent IPv4/IPv6 pings;
- sustained cellular traffic with RMNET IPv4/IPv6, route and DNS checks;
- five WLAN-to-cellular-to-WLAN handoffs;
- normal application traffic and Android data-usage/accounting views;
- a bounded Data Saver off/on check if the UI permits it without changing
  persistent policy intent;
- `netd`, NetworkStack, ConnectivityService and system_server health.

Pass requires the installed quota2 rules to remain present, counters to behave
sanely under traffic, no counter disappearance/corruption, no persistent
network failure, and no relevant netfilter/procfs/kernel warning. Do not force
allocator failure, memory pressure or procfs failure to manufacture the exact
race.

## Standard regression gates

- WLAN 6135 MHz WPA3-SAE and Internet;
- LTE HOME/IN_SERVICE, RMNET dual stack, routes, numeric IP and DNS;
- Bluetooth off/on and existing HID reconnect when available;
- NFC service, Wallet/HCE/eSE1;
- camera capture and clear ringtone/media audio;
- ADB/USB enumeration, normal UI/framework operation;
- deep idle and resume 5/5;
- module load contract 82 entries with `wwan.ko` at entry 28;
- full dmesg/logcat scan for netfilter, quota2, refcount, UAF, Oops, BUG,
  panic, KASAN/UBSAN, RCU/hung-task and framework crash indicators.

If the exact procfs-creation-failure plus concurrent-reference interleaving is
not proven, record:

`NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED`.

On a critical regression, restore exact B07 boot
`1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1`,
verify read-back, and do not begin B09.

No flash is authorized by this plan itself.

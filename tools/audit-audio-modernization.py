#!/usr/bin/env python3
"""Emit the physically qualified Canoe normal-boot audio module closure."""

from __future__ import annotations

import argparse
import csv
import pathlib
import subprocess


QCOM = "vendor/qcom/opensource/audio-kernel"
OPLUS = "vendor/oplus/kernel/audio"

# module: (source path, build target, hardware/runtime role)
MODULES = {
    "q6_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_q6_dlkm", "Q6 audio event interface"),
    "spf_core_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_spf_core_dlkm", "SPF/APM GPR service"),
    "audpkt_ion_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_audpkt_ion_dlkm", "audio packet memory"),
    "q6_notifier_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_q6_notifier_dlkm", "ADSP notifier/SSR"),
    "adsp_loader_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_adsp_loader_dlkm", "ADSP loader coordination"),
    "audio_prm_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_audio_prm_dlkm", "audio power/resource manager"),
    "q6_pdr_dlkm": (f"{QCOM}/dsp", "//vendor/qcom/opensource/audio-kernel:canoe_perf_q6_pdr_dlkm", "ADSP process-domain restart"),
    "gpr_dlkm": (f"{QCOM}/ipc/gpr-lite.c", "//vendor/qcom/opensource/audio-kernel:canoe_perf_gpr_dlkm", "GPR RPMSG transport/provider"),
    "audio_pkt_dlkm": (f"{QCOM}/ipc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_audio_pkt_dlkm", "audio packet GPR client"),
    "pinctrl_lpi_dlkm": (f"{QCOM}/soc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_pinctrl_lpi_dlkm", "low-power audio pinctrl"),
    "swr_dlkm": (f"{QCOM}/soc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_swr_dlkm", "SoundWire bus core"),
    "swr_ctrl_dlkm": (f"{QCOM}/soc/swr-mstr-ctrl.c", "//vendor/qcom/opensource/audio-kernel:canoe_perf_swr_ctrl_dlkm", "SoundWire master controller"),
    "snd_event_dlkm": (f"{QCOM}/soc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_snd_event_dlkm", "audio sound-event/SSR provider"),
    "machine_dlkm": (f"{QCOM}/asoc/audio_machine.c", "//vendor/qcom/opensource/audio-kernel:canoe_perf_machine_dlkm", "Canoe ASoC machine/routing"),
    "wcd_core_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd_core_dlkm", "WCD codec core"),
    "mbhc_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_mbhc_dlkm", "headset/MBHC detection"),
    "swr_dmic_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_swr_dmic_dlkm", "SoundWire digital microphones"),
    "wcd9xxx_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd9xxx_dlkm", "WCD IRQ/core helper"),
    "swr_haptics_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_swr_haptics_dlkm", "SoundWire haptics path"),
    "lpass_cdc_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_dlkm", "LPASS codec parent"),
    "lpass_cdc_wsa_macro_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_wsa_macro_dlkm", "LPASS WSA macro"),
    "lpass_cdc_wsa2_macro_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_wsa2_macro_dlkm", "LPASS WSA2/haptics macro"),
    "lpass_cdc_va_macro_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_va_macro_dlkm", "LPASS voice-assistant macro"),
    "lpass_cdc_rx_macro_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_rx_macro_dlkm", "LPASS receive macro"),
    "lpass_cdc_tx_macro_dlkm": (f"{QCOM}/asoc/codecs/lpass-cdc/lpass-cdc-tx-macro.c", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_cdc_tx_macro_dlkm", "LPASS microphone/transmit macro"),
    "wsa883x_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wsa883x_dlkm", "WSA883x codec support"),
    "wsa884x_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wsa884x_dlkm", "WSA884x codec support"),
    "wcd938x_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd938x_dlkm", "WCD938x codec support"),
    "wcd938x_slave_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd938x_slave_dlkm", "WCD938x SoundWire slave"),
    "wcd939x_dlkm": (f"{QCOM}/asoc/codecs/wcd939x", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd939x_dlkm", "active WCD9395 codec"),
    "wcd939x_slave_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd939x_slave_dlkm", "active WCD939x SoundWire slave"),
    "wcd9378_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd9378_dlkm", "WCD9378 codec support"),
    "wcd9378_slave_dlkm": (f"{QCOM}/asoc/codecs", "//vendor/qcom/opensource/audio-kernel:canoe_perf_wcd9378_slave_dlkm", "WCD9378 SoundWire slave"),
    "lpass_bt_swr_dlkm": (f"{QCOM}/asoc", "//vendor/qcom/opensource/audio-kernel:canoe_perf_lpass_bt_swr_dlkm", "Bluetooth audio SoundWire bridge"),
    "oplus_audio_tfa98xx_v6": (f"{OPLUS}/codecs/tfa98xx-v6", "//vendor/qcom/opensource/audio-kernel:canoe_perf_oplus_audio_tfa98xx_v6", "active NXP TFA986x speakers"),
    "oplus_audio_aw882xx": (f"{OPLUS}/codecs/aw882xx_v1.13.0", "//vendor/qcom/opensource/audio-kernel:canoe_perf_oplus_audio_aw882xx", "packaged alternate speaker codec"),
    "oplus_audio_daemon": (f"{OPLUS}/audio_daemon", "//vendor/qcom/opensource/audio-kernel:canoe_perf_oplus_audio_daemon", "Oplus audio SSR/feedback daemon"),
    "oplus_audio_netlink": (f"{OPLUS}/audio_netlink", "//vendor/qcom/opensource/audio-kernel:canoe_perf_oplus_audio_netlink", "Oplus audio netlink"),
    "oplus_typec_switch_i2c": (f"{OPLUS}/typec_switch", "//vendor/qcom/opensource/audio-kernel:canoe_perf_oplus_typec_switch_i2c", "USB-C analog/audio switch"),
    "adsp_sleepmon": ("soc-repo/drivers/soc/qcom/adsp_sleepmon.c", "//soc-repo:canoe_perf_adsp_sleepmon", "ADSP sleep monitor"),
    "slimbus": ("soc-repo/drivers/slimbus", "//soc-repo:canoe_perf_slimbus", "Slimbus provider"),
    "oplus_wifi_wsa": ("vendor/oplus/kernel/wifi/oplus_wifi_wsa", "//vendor/oplus/kernel/wifi:canoe_perf_oplus_wifi_wsa", "shared WLAN smart-antenna helper"),
    "wcd_usbss_i2c": ("soc-repo/drivers/soc/qcom", "//soc-repo:canoe_perf_wcd_usbss_i2c", "USB-C audio switch provider"),
    "snd_usb_audio_qmi": ("soc-repo/sound/usb", "//soc-repo:canoe_perf_snd_usb_audio_qmi", "USB audio QMI service"),
}


def modinfo(path: pathlib.Path, field: str) -> str:
    result = subprocess.run(
        ["modinfo", "-F", field, str(path)], check=True, text=True,
        stdout=subprocess.PIPE,
    )
    return ";".join(line for line in result.stdout.splitlines() if line)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--module-contract", required=True, type=pathlib.Path)
    parser.add_argument("--proc-modules", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    with args.module_contract.open(encoding="utf-8", newline="") as stream:
        contract = {row["module"]: row for row in csv.DictReader(stream, delimiter="\t")}
    loaded = {
        line.split()[0] for line in args.proc_modules.read_text(encoding="utf-8").splitlines()
        if line.strip()
    }
    missing = sorted(set(MODULES) - set(contract))
    if missing:
        raise SystemExit(f"modules absent from qualified contract: {', '.join(missing)}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "module", "partition", "source_path", "build_target", "loaded",
        "normal_boot", "dependencies", "imports", "exports", "module_parameters",
        "signer", "vermagic", "sha256", "hardware_role",
    ]
    with args.output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for name, (source, target, role) in MODULES.items():
            row = contract[name]
            module_path = pathlib.Path(row["path"])
            is_loaded = name in loaded
            writer.writerow({
                "module": f"{name}.ko",
                "partition": "vendor_dlkm",
                "source_path": source,
                "build_target": target,
                "loaded": "yes" if is_loaded else "no",
                "normal_boot": "yes" if is_loaded else "packaged/not observed",
                "dependencies": row["depends"] or "none",
                "imports": row["imports"],
                "exports": row["exports"],
                "module_parameters": modinfo(module_path, "parm") or "none",
                "signer": modinfo(module_path, "signer") or "unsigned retained stock",
                "vermagic": modinfo(module_path, "vermagic"),
                "sha256": row["sha256"],
                "hardware_role": role,
            })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

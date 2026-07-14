load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        combkey_monitor_ko_deps = [
            "//vendor/oplus/kernel/dfr:oplus_bsp_dfr_keyevent_handler",
            "//vendor/oplus/kernel/dfr:oplus_bsp_dfr_theia",
            "//vendor/oplus/kernel/dft/bazel:oplus_bsp_dft_kernel_fb",
        ]
        hung_task_enhance_ko_deps = [
            "//vendor/oplus/kernel/dfr:oplus_bsp_dfr_theia",
            "//vendor/oplus/kernel/boot:oplus_bsp_boot_projectinfo",
        ]
        shutdown_detect_ko_deps = [
            "//vendor/oplus/kernel/boot:oplus_bsp_boot_projectinfo",
        ]
        theia_ko_deps = [
            "//soc-repo:{}/drivers/soc/qcom/panel_event_notifier".format(kernel_build_variant)
        ]
        dump_device_info_ko_deps = [
            "//vendor/oplus/kernel/boot:oplus_bsp_boot_projectinfo",
            "//vendor/oplus/kernel/boot:oplusboot",
            "//soc-repo:{}/drivers/soc/qcom/debug_symbol".format(kernel_build_variant),
        ]
        dump_reason_ko_deps = [
            "//vendor/oplus/kernel/dfr:oplus_bsp_dfr_dump_device_info",
            "//soc-repo:{}/drivers/soc/qcom/smem".format(kernel_build_variant),
        ]
        pmic_watchdog_ko_deps = [
            "//vendor/oplus/kernel/boot:oplus_bsp_boot_projectinfo",
            "//soc-repo:{}/drivers/input/misc/qpnp-power-on".format(kernel_build_variant),
         ]

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_combkey_monitor",
        srcs = native.glob([
            "**/*.h",
            "common/combkey_monitor/combkey_monitor.c",
        ]),
        includes = ["."],
        ko_deps = combkey_monitor_ko_deps,
        local_defines = ["CONFIG_OPLUS_FEATURE_THEIA","CONFIG_OPLUS_FEATURE_KEYEVENT_HANDLER"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_hung_task_enhance",
        srcs = native.glob([
            "**/*.h",
            "common/hung_task_enhance/hung_task_enhance.c",
        ]),
        includes = ["."],
        ko_deps = hung_task_enhance_ko_deps,
        local_defines = [
            "CONFIG_OPLUS_FEATURE_HUNG_TASK_ENHANCE",
            "CONFIG_OPLUS_FEATURE_THEIA",
            "CONFIG_OPLUS_FEATURE_DEATH_HEALER",
            "CONFIG_OPLUS_BSP_DFR_USERSPACE_BACKTRACE",
            "CONFIG_OPLUS_FEATURE_HUNGTASK_GAIA",
        ],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_init_watchdog",
        srcs = native.glob([
            "**/*.h",
            "common/init_watchdog/init_watchdog.c",
        ]),
        includes = ["."],
        ko_deps = [
            "//vendor/oplus/kernel/dfr:oplus_bsp_dfr_theia",
        ],
        local_defines = ["CONFIG_OPLUS_FEATURE_THEIA"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_keyevent_handler",
        srcs = native.glob([
            "**/*.h",
            "common/keyevent_handler/keyevent_handler.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_last_boot_reason",
        srcs = native.glob([
            "**/*.h",
            "common/last_boot_reason/last_boot_reason.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_SHUTDOWN_DETECT"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_fdleak_check",
        srcs = native.glob([
            "**/*.h",
            "common/oplus_fdleak/oplus_fdleak_check.c",
        ]),
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
#        header_deps = [
#            "//vendor/oplus/kernel/cpu:config_headers",
#        ],
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_oplus_saupwk",
        srcs = native.glob([
            "**/*.h",
            "common/oplus_saupwk/oplus_saupwk.c",
        ]),
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_SAUPWK"],
    )


    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_shutdown_detect",
        srcs = native.glob([
            "**/*.h",
            "common/shutdown_detect/shutdown_detect.c",
        ]),
        includes = ["."],
        ko_deps = shutdown_detect_ko_deps,
        local_defines = ["CONFIG_OPLUS_FEATURE_SHUTDOWN_DETECT","CONFIG_OPLUS_BSP_DFR_USERSPACE_BACKTRACE"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_ubt",
        srcs = native.glob([
            "**/*.h",
            "common/oplus_bsp_dfr_ubt/oplus_bsp_dfr_ubt.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_theia",
        srcs = native.glob([
            "**/*.h",
            "common/theia/black_screen_check.c",
            "common/theia/bright_screen_check.c",
            "common/theia/theia_kevent_kernel.c",
            "common/theia/powerkey_monitor.c",
            "common/theia/theia_send_event.c",
        ]),
        conditional_defines = {
            "qcom": ["CONFIG_QCOM_PANEL_EVENT_NOTIFIER"],
        },
        includes = ["."],
        ko_deps = theia_ko_deps,
        local_defines = ["CONFIG_OPLUS_FEATURE_THEIA_MODULE"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_force_shutdown",
        srcs = native.glob([
            "**/*.h",
        ]),
        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK": {
                False: ["qcom/force_shutdown/force_shutdown.c"],
            }
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_pmic_monitor",
        srcs = native.glob([
            "**/*.h",
        ]),
        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK": {
                False: ["qcom/qcom_pmic_monitor/oplus_pmic_info_smem.c",
                "qcom/qcom_pmic_monitor/main.c",
                "qcom/qcom_pmic_monitor/oplus_pmic_machine_state.c",
                "qcom/qcom_pmic_monitor/oplus_ocp_dev.c",
                "qcom/qcom_pmic_monitor/oplus_ocp_state_nvmem.c"],
            }
        },
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_dump_device_info",
        srcs = native.glob([
            "**/*.h",
            "qcom/dump_device_info/dump_device_info.c",
        ]),
        ko_deps = dump_device_info_ko_deps,
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_dump_reason",
        srcs = native.glob([
            "**/*.h",
            "qcom/dump_reason/dump_reason.c",
        ]),
        ko_deps = dump_reason_ko_deps,
        copts = ["-DCONFIG_QCOM_SMEM"],
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_pmic_watchdog",
        srcs = native.glob([
            "**/*.h",
            "qcom/qcom_pmicwd/qcom_pmicwd.c",
            "qcom/qcom_pmicwd/qcom_pwkpwr.c",
	    "qcom/qcom_pmicwd/qcom_pmicwd_inject.c",
        ]),
        ko_deps = pmic_watchdog_ko_deps,
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_inject",
        srcs = native.glob([
            "**/*.h",
            "fault_inject/common/oplus_inject_hook.c",
            "fault_inject/common/oplus_inject_proc.c",
        ]),
        includes = ["."],
        conditional_build = {
            "OPLUS_FEATURE_BSP_DRV_INJECT_TEST": "1",
        },
    )

    define_oplus_ddk_module(
        name = "oplus_inject_aw8692x",
        srcs = native.glob([
            "**/*.h",
        ]),
        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK": {
                True:  ["fault_inject/vibrator/oplus_inject_aw8692x.c"],
                False: ["fault_inject/vibrator/oplus_inject_haptics.c"],
            },
        },
        ko_deps = [
            "//vendor/oplus/kernel/dfr:oplus_inject",
            "//vendor/oplus/kernel/vibrator/bazel:oplus_bsp_haptic_feedback",
        ],
        includes = ["."],
        conditional_build = {
            "OPLUS_FEATURE_BSP_DRV_INJECT_TEST": "1",
        },
        local_defines = ["CONFIG_HAPTIC_FEEDBACK_MODULE"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_ordump",
        srcs = native.glob([
            "**/*.h",
            "qcom/oplus_ordump/ordump.c",
        ]),
        copts = ["-DCONFIG_QCOM_SMEM"],
        includes = ["."],
        ko_deps = [
            "//soc-repo:{}/drivers/soc/qcom/smem".format(kernel_build_variant),
        ],
        local_defines = ["CONFIG_OPLUS_FEATURE_FULLDUMP_BACK"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_kp_freeze_detect",
        srcs = native.glob([
            "**/*.h",
            "common/kp_freeze_detect/kp_freeze_detect.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_KP_FREEZE_DETECT"],
    )

    define_oplus_ddk_module(
        name = "mtk_wdt",
        srcs = native.glob([
            "**/**/*.h",
            "qcom/mtk_wdt/mtk_wdt.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_MTK_WDT"],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_bsp_dfr",
        module_list = [
            "oplus_bsp_dfr_combkey_monitor",
            "oplus_bsp_dfr_hung_task_enhance",
            "oplus_bsp_dfr_init_watchdog",
            "oplus_bsp_dfr_keyevent_handler",
            "oplus_bsp_dfr_last_boot_reason",
            "oplus_bsp_dfr_fdleak_check",
            "oplus_bsp_dfr_shutdown_detect",
            "oplus_bsp_dfr_ubt",
            "oplus_bsp_dfr_theia",
            "oplus_bsp_dfr_force_shutdown",
            "oplus_bsp_dfr_pmic_monitor",
            "oplus_bsp_dfr_dump_device_info",
            "oplus_bsp_dfr_dump_reason",
            "oplus_bsp_dfr_pmic_watchdog",
            "oplus_inject",
            "oplus_inject_aw8692x",
            "oplus_bsp_dfr_ordump",
            "oplus_bsp_dfr_kp_freeze_detect",
            "mtk_wdt",
        ],
        conditional_builds = {
            "oplus_inject_aw8692x": {
                "OPLUS_FEATURE_BSP_DRV_INJECT_TEST": "1",
            },
            "oplus_inject": {
                "OPLUS_FEATURE_BSP_DRV_INJECT_TEST": "1",
            },
        },
    )

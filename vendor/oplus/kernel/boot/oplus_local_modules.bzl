load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        phoenix_ko_deps = [
           ":oplus_bsp_bootmode",
           ":oplus_bsp_boot_projectinfo",
        ]
        kmsg_wb_ko_deps = [
            ":oplus_bsp_dfr_phoenix",
            ":oplus_bsp_boot_projectinfo",
        ]
    else :
        phoenix_ko_deps = [
            "//kernel_device_modules-{}/drivers/soc/oplus/boot:oplus_bsp_boot_projectinfo".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/boot_common:mtk_boot_common".format(kernel_version),
        ]
        kmsg_wb_ko_deps = [
            "//vendor/oplus/kernel/boot:oplus_bsp_dfr_phoenix",
            "//kernel_device_modules-{}/drivers/soc/oplus/boot:oplus_bsp_boot_projectinfo".format(kernel_version),
        ]

    define_oplus_ddk_module(
        name = "saupwk",
        srcs = native.glob([
            "cmdline_parser/saupwk.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplusboot",
        srcs = native.glob([
            "cmdline_parser/oplusboot.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_ftm_mode",
        srcs = native.glob([
            "cmdline_parser/oplus_ftm_mode.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_charger_present",
        srcs = native.glob([
            "cmdline_parser/oplus_charger_present.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "buildvariant",
        srcs = native.glob([
            "cmdline_parser/buildvariant.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "cdt_integrity",
        srcs = native.glob([
            "cmdline_parser/cdt_integrity.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_bootargs",
        srcs = native.glob([
            "cmdline_parser/oplus_bootargs.c",
        ]),
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_bsp_bootmode",
        srcs = native.glob([
            "bootmode/boot_mode.c",
        ]),
        ko_deps = [
            ":oplus_ftm_mode",
            ":oplusboot",
            ":oplus_charger_present",
        ],
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_bsp_bootloader_log",
        srcs = native.glob([
            "bootloader_log/bootloader_log.c",
        ]),
        includes = ["include"],
    )

    if bazel_support_platform == "qcom" :
        ko_deps = [
                ":buildvariant",
                ":oplusboot",
                "//soc-repo:{}/drivers/soc/qcom/smem".format(kernel_build_variant),
        ]
        copts = ["-DCONFIG_QCOM_SMEM"]
    else :
        ko_deps = [
                ":buildvariant",
                ":oplusboot",
        ]
        copts = []

    define_oplus_ddk_module(
        name = "oplus_bsp_boot_projectinfo",
        srcs = native.glob([
            "include/oplus_project.h",
        ]),

        ko_deps = ko_deps,

        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK": {
                True: ["oplus_projectinfo/mtk/oplus_project.c"],
                False: ["oplus_projectinfo/qcom/oplus_project.c"],
            }
        },
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
        copts = copts,
        includes = ["include"],
    )
    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_shutdown_speed",
        srcs = native.glob([
            "oplus_phoenix/shutdown_speed.c",
        ]),
        includes = ["include"],
    )

    define_oplus_ddk_module(
         name = "oplus_bsp_dfr_kmsg_wb",
         srcs = native.glob([
            "**/*.h",
            "oplus_phoenix/oplus_kmsg_wb.c",
         ]),
         ko_deps = kmsg_wb_ko_deps,
         includes = ["."],
     )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_reboot_speed",
        srcs = native.glob([
            "**/*.h",
            "oplus_phoenix/phoenix_reboot_speed.c",
        ]),
        ko_deps = [
           ":oplus_bsp_dfr_shutdown_speed",
        ],
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_phoenix",
        srcs = native.glob([
            "**/*.h",
            "oplus_phoenix/op_bootprof.c",
            "oplus_phoenix/phoenix_dump.c",
            "oplus_phoenix/phoenix_watchdog.c",
            "oplus_phoenix/phoenix_base.c",
        ]),
        ko_deps = phoenix_ko_deps,
        includes = ["."],
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
        local_defines = ["TRACK_TASK_COMM","CONFIG_OPLUS_FEATURE_PHOENIX_MODULE"],
    )
    define_oplus_ddk_module(
        name = "oplus_bsp_dfr_qcom_enhance_watchdog",
        srcs = native.glob([
            "**/*.h",
            "qcom_watchdog/qcom_enhance_watchdog.c",
        ]),
        includes = ["."],
	local_defines = ["CONFIG_OPLUS_FEATURE_QCOM_WATCHDOG_MODULE"],
    )

    define_oplus_ddk_module(
        name = "tango32",
        srcs = native.glob([
            "htb/*.h",
            "htb/*.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_TANGO32"],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_bsp_boot",
        module_list = [
            "saupwk",
            "oplusboot",
            "oplus_ftm_mode",
            "oplus_charger_present",
            "buildvariant",
            "cdt_integrity",
            "oplus_bsp_bootmode",
            "oplus_bsp_bootloader_log",
            "oplus_bsp_boot_projectinfo",
            "oplus_bsp_dfr_reboot_speed",
            "oplus_bsp_dfr_phoenix",
            "oplus_bsp_dfr_shutdown_speed",
            "oplus_bsp_dfr_qcom_enhance_watchdog",
            "oplus_bsp_dfr_kmsg_wb",
            "tango32",
        ],
    )

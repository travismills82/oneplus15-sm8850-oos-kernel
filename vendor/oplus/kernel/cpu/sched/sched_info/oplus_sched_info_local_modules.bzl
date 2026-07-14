load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_sched_info_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
                    "//soc-repo:{}/kernel/sched/walt/sched-walt".format(kernel_build_variant)]
        copts = ["-DCONFIG_SCHED_WALT"]
        kconfig = None
        defconfig = None
        ddk_config = "//soc-repo:{}_config".format(kernel_build_variant)
    else :
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"
        ]
        copts = []
        kconfig = "sched/Kconfig"
        defconfig = "build/defconfig/{}/sched/sched_configs".format(target)
        ddk_config = None

    define_oplus_ddk_module(
        name = "oplus_bsp_schedinfo",
        srcs = native.glob([
            "**/*.h",
            "sched/sched_info/oplus_sched_info.c",
            "sched/sched_info/osi_amu.c",
            "sched/sched_info/osi_base.c",
            "sched/sched_info/osi_cpuload.c",
            "sched/sched_info/osi_cpuloadmonitor.c",
            "sched/sched_info/osi_debug.c",
            "sched/sched_info/osi_enable.c",
            "sched/sched_info/osi_freq.c",
            "sched/sched_info/osi_healthinfo.c",
            "sched/sched_info/osi_hotthread.c",
            "sched/sched_info/osi_loadinfo.c",
            "sched/sched_info/osi_memory_monitor.c",
            "sched/sched_info/osi_netlink.c",
            "sched/sched_info/osi_tasktrack.c",
            "sched/sched_info/osi_topology.c",
            "sched/sched_info/osi_version.c",
        ]),
        local_defines = [],
        kconfig = kconfig,
        defconfig = defconfig,
        config = ddk_config,
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM","CONFIG_SCHED_WALT"],
        },
        ko_deps = ko_deps,
        copts = copts,
        includes = ["."],
    )

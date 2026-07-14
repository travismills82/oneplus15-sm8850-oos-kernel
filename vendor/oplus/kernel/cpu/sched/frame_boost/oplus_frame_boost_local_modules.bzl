load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_frame_boost_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        ko_deps = [
        ]
        copts = ["-DCONFIG_SCHED_WALT"]
        kconfig = None
        defconfig = None
        ddk_config = "//soc-repo:{}_config".format(kernel_build_variant)
    else :
        ko_deps = [
        ]
        copts = []
        kconfig = "sched/Kconfig"
        defconfig = "build/defconfig/{}/sched/sched_configs".format(target)
        ddk_config = None

    define_oplus_ddk_module(
        name = "oplus_bsp_frame_boost",
        srcs = native.glob([
            "**/*.h",
            "sched/frame_boost/cluster_boost.c",
            "sched/frame_boost/frame_boost.c",
            "sched/frame_boost/frame_debug.c",
            "sched/frame_boost/frame_group.c",
            "sched/frame_boost/frame_info.c",
            "sched/frame_boost/frame_sysctl.c",
            "sched/frame_boost/frame_timer.c",
        ]),
        local_defines = [],
        kconfig = kconfig,
        defconfig = defconfig,
        config = ddk_config,
        conditional_srcs = {
            "CONFIG_OPLUS_FEATURE_SCHED_CFBT": {
                True: [
                    "sched/frame_boost/cfbt_boost.c",
                    "sched/frame_boost/cfbt_config.c",
                    "sched/frame_boost/cfbt_rescue.c",
                    "sched/frame_boost/cfbt_state.c",
                    "sched/frame_boost/cfbt_trace.c"
                ],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_FDRIVE": {
                True: ["sched/frame_boost/cfbt_cx_opt.c"],
            },
        },
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM","CONFIG_SCHED_WALT"],
        },
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"],
        copts = copts,
        includes = ["."],
    )
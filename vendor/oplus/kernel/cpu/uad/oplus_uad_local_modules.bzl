load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
    
def define_oplus_uad_local_modules():
    
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        ko_deps = [
        ]
        copts = ["-DCONFIG_SCHED_WALT"]
    else :
        ko_deps = [
        ]
        copts = []

    define_oplus_ddk_module(
        name = "ua_cpu_ioctl",
        srcs = native.glob([
            "**/*.h",
            "uad/ua_ioctl/*.h",
            "uad/ua_ioctl/ua_ioctl_common.c",
            "uad/ua_ioctl/touch_ioctl.c",
        ]),
        local_defines = [],
        kconfig = "uad/Kconfig",
        defconfig = "build/defconfig/{}/uad_configs".format(target),
        conditional_srcs = {
            "CONFIG_OPLUS_FEATURE_FRAME_BOOST": {
                True: [
                    "uad/ua_ioctl/frame_ioctl.c",
                ],
            },
        },
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM","CONFIG_SCHED_WALT"],
        },
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_frame_boost"],
        copts = copts,
        includes = ["."],
    )

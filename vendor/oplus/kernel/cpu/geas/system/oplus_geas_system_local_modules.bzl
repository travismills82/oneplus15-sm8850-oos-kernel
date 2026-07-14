load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform", "oplus_ddk_get_kernel_version")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_geas_system_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    tv = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if target == "k6993v1_64" :
        kernelconfig = "geas/system/kernel_config/k6993v1_64/geas_configs"
    elif target == "canoe" :
        kernelconfig = "geas/system/kernel_config/canoe/geas_configs"
    else :
        kernelconfig = "geas/system/kernel_config/default/geas_configs"

    if bazel_support_platform == "qcom":
        deps = [
            "//vendor/oplus/kernel/cpu:oplus_bsp_frame_boost",
            "//vendor/oplus/kernel/cpu:oplus_bsp_game_opt",
            "//vendor/oplus/kernel/cpu:ua_cpu_ioctl",
            "//vendor/qcom/opensource/graphics-kernel:{}_msm_kgsl".format(tv),
            "//soc-repo:{}/drivers/soc/qcom/dcvs/bwmon".format(tv),
            "//soc-repo:{}/drivers/soc/qcom/dcvs/memlat".format(tv),
            "//soc-repo:{}/drivers/soc/qcom/dcvs/qcom-pmu-lib".format(tv),
            "//soc-repo:{}/drivers/soc/qcom/dcvs/qcom-dcvs".format(tv),
        ]
    elif bazel_support_platform == "mtk":
        deps = [
            "//vendor/oplus/kernel/cpu:oplus_bsp_game_opt",
            "//kernel_device_modules-{}/drivers/misc/mediatek/dvfsrc:mtk-dvfsrc-helper".format(kernel_version),
        ]
    else :
        deps = []

    define_oplus_ddk_module(
        name = "oplus_bsp_geas_system",
        srcs = native.glob([
            "geas/system/geas.h",
        ]),
        kconfig = "geas/system/Kconfig",
        defconfig = kernelconfig,
        conditional_srcs = {
            "CONFIG_OPLUS_FEATURE_GEAS": {
                True:["geas/system/geas.c"],
                False:["geas/system/empty.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_FDRIVE": {
                True:["geas/system/bwmon_geas.h",
                      "geas/system/geas_frame_drive.c",
                      "geas/system/geas_sysctrl.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_BWMON": {
                True:["geas/system/geas_bwmon.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_GPU": {
                True:["geas/system/geas_gpu.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_EMI": {
                True:["geas/system/geas_emi.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_MEMLAT": {
                True:["geas/system/geas_memlat.c"],
            },
            "CONFIG_OPLUS_FEATURE_GEAS_NPU": {
                True:["geas/system/geas_gpu.c"],
            },
        },
        ko_deps = deps,
    )


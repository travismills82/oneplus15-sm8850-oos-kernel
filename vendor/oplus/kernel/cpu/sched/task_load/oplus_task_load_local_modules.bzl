load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_task_load_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        ko_deps = ["//soc-repo:{}/kernel/sched/walt/sched-walt".format(kernel_build_variant)]
        copts = ["-DCONFIG_SCHED_WALT"]
    else :
        ko_deps = [
        ]
        copts = []

    define_oplus_ddk_module(
        name = "oplus_bsp_task_load",
        srcs = native.glob([
            "sched/task_load/task_load.c",
            "sched/task_load/task_load.h",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_TASK_LOAD"],
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM","CONFIG_SCHED_WALT"],
        },
        copts = copts,
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
            "//soc-repo:{}/kernel/sched/walt/sched-walt".format(kernel_build_variant)],
    )
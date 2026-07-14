load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_task_sched_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "qcom" :
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_frame_boost",
                    "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"]
        copts = ["-DCONFIG_SCHED_WALT"]
    else :
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"]
        copts = []

    define_oplus_ddk_module(
        name = "oplus_bsp_task_sched",
        srcs = native.glob([
            "**/*.h",
            "sched/task_sched/task_sched_info.c",
            "sched/task_sched/task_sched_info.h",
        ]),
        includes = ["."],
        ko_deps = ko_deps,
    )

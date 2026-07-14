load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)

    if bazel_support_platform == "qcom" :
        ko_deps = ["//soc-repo:{}/drivers/firmware/qcom/qcom-scm".format(kernel_build_variant)]
    else :
        ko_deps = []

    define_oplus_ddk_module(
        name = "oplus_secure_common",
        srcs = native.glob([
            "oplus_secure_common.c",
            "*.h",
        ]),
        includes = ["."],
        ko_deps = ko_deps,
        conditional_defines = {
            "qcom":  ["QCOM_PLATFORM", "QCOM_QSEELOG_ENCRYPT"],
            "mtk":   ["MTK_PLATFORM", "CONFIG_OPLUS_SECURE_COMMON_MODULE"],
        },
    )

    ddk_copy_to_dist_dir(
        name = "oplus_securecommon",
        module_list = [
            "oplus_secure_common",
        ],
    )


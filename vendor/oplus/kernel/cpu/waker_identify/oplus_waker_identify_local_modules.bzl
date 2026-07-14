load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_waker_identify_local_modules():

    define_oplus_ddk_module(
        name = "oplus_bsp_waker_identify",
        srcs = native.glob([
            "waker_identify/*.h",
            "waker_identify/*.c",
        ]),
        includes = ["."],
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
    )

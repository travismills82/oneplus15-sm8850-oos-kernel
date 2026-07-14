load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_omrg_local_modules():
    target = oplus_ddk_get_target()
    if target == "k6993v1_64" :
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_freq_qos_arbiter",]
    else :
        ko_deps = []
    copts = []
    kconfig = "oplus_omrg/Kconfig"
    defconfig = "build/defconfig/{}/omrg_configs".format(target)

    define_oplus_ddk_module(
        name = "oplus_bsp_omrg",
        srcs = native.glob([
            "oplus_omrg/oplus_omrg.c",
            "oplus_omrg/oplus_omrg.h",
            "oplus_omrg/oplus_omrg_trace.h",
        ]),
        includes = ["."],
        kconfig = kconfig,
        ko_deps = ko_deps,
        defconfig = defconfig,
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
        },
    )

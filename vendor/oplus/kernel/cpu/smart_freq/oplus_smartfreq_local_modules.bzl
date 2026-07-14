load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_smart_freq_local_modules():
    target = oplus_ddk_get_target()

    ko_deps = []
    copts = []
    kconfig = "smart_freq/Kconfig"
    defconfig = "build/defconfig/{}/smart_freq_configs".format(target)

    define_oplus_ddk_module(
        name = "oplus_bsp_smart_freq",
        srcs = native.glob([
            "smart_freq/*.h",
            "smart_freq/*.c",
        ]),
        includes = ["."],
        kconfig = kconfig,
        ko_deps = ko_deps,
        defconfig = defconfig,
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
        },
    )

load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "bazel_support_platform", "oplus_ddk_get_target", "oplus_ddk_get_kernel_version")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_slc_local_modules():
    target = oplus_ddk_get_target()
    kernel_version = oplus_ddk_get_kernel_version()

    if bazel_support_platform == "mtk" :
        copts = [
            "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat",
        ]
        kconfig = "oplus_slc/Kconfig"
        defconfig = "build/defconfig/{}/slc_configs".format(target)
        header_deps = [
            "//kernel_device_modules-{}:all_mgk_headers".format(kernel_version),
            "//kernel_device_modules-{}/drivers/gpu/mediatek/gpufreq/v2:gpufreq_public_headers".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/qos:ddk_public_headers".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/slbc:headers".format(kernel_version),
        ]
        ko_deps = [
            "//kernel_device_modules-{}/drivers/gpu/mediatek/gpufreq/v2:mtk_gpufreq_mt6993".format(kernel_version),
            "//kernel_device_modules-{}/drivers/gpu/mediatek/gpufreq/v2:mtk_gpufreq_wrapper".format(kernel_version),
            "//kernel_device_modules-{}/drivers/gpu/mediatek/ged:ged".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/qos:mtk_qos".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/slbc:mtk_slbc".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/slbc:slbc_ipi".format(kernel_version),
            "//kernel_device_modules-{}/drivers/misc/mediatek/dvfsrc:mtk-dvfsrc-helper".format(kernel_version),
            "//kernel_device_modules-{}/drivers/memory/mediatek:mtk_dramc".format(kernel_version),
        ]
        local_defines = [
            "CONFIG_MTK_SLBC_MT6989",
            "CONFIG_MTK_SLBC_MT6991",
            "CONFIG_MTK_SLBC_MT6993",
        ]

    define_oplus_ddk_module(
        name = "oplus_slc",
        srcs = native.glob([
            "oplus_slc/*.h",
            "oplus_slc/*.c",
        ]),
        includes = ["."],
        kconfig = kconfig,
        defconfig = defconfig,
        ko_deps = ko_deps,
        header_deps = header_deps,
        local_defines = local_defines,
    )

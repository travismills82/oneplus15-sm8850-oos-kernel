load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_cpufreq_bouncing_local_modules():
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
        copts = []
        kconfig = "cpufreq_bouncing/Kconfig"
        defconfig = "build/defconfig/{}/cpufreq_bouncing_configs".format(target)
        ddk_config = None
        if target == "k6993v1_64" :
            ko_deps = ["//vendor/oplus/kernel/cpu:oplus_freq_qos_arbiter",]
        else :
            ko_deps = []

    define_oplus_ddk_module(
        name = "cpufreq_bouncing",
        srcs = native.glob([
            "cpufreq_bouncing/*.h",
        ]),
        includes = ["."],
        conditional_srcs = {
            "CONFIG_OPLUS_DDK_MTK": {
                True: [
                    "cpufreq_bouncing/cpufreq_bouncing_mtk.c",
                ],
                False: [
                    "cpufreq_bouncing/cpufreq_bouncing.c",
                ],
            },
        },
        kconfig = kconfig,
        defconfig = defconfig,
        config = ddk_config,
        ko_deps = ko_deps,
        conditional_defines = {
            "mtk":  ["CONFIG_OPLUS_SYSTEM_KERNEL_MTK"],
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM","CONFIG_SCHED_WALT"],
        },
    )

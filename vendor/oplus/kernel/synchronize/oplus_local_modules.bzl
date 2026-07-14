load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():

    define_oplus_ddk_module(
        name = "oplus_locking_strategy",
        srcs = native.glob([
        "futex.c",
        "locking_main.c",
        "mutex.c",
        "rwsem.c",
        "sysfs.c",
        "kern_lock_stat.c",
        "*.h",
        "oplus_locking.c",
        "sched_assist_locking.c",
        ]),
        conditional_srcs = {
            "CONFIG_LOCKING_PROTECT": {
                True:  ["sched_assist_locking.c"],
            }
        },
        includes = ["."],
        local_defines = [
            "CONFIG_OPLUS_LOCKING_STRATEGY",
            "CONFIG_OPLUS_LOCKING_OSQ",
            "CONFIG_OPLUS_LOCKING_MONITOR",
            "CONFIG_LOCKING_PROTECT",
            "CONFIG_PCPU_RWSEM_LOCKING_PROTECT",
        ],
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"],
    )

    ddk_headers(
        name = "config_headers",
        hdrs  = native.glob([
            "**/*.h",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_lock_torture",
        srcs = native.glob([
        "locktorture.c",
        "*.h",
        ]),
        includes = ["."],
        local_defines = [
            "CONFIG_OPLUS_LOCKING_STRATEGY",
            "CONFIG_OPLUS_LOCKING_OSQ",
            "CONFIG_OPLUS_LOCKING_MONITOR",
            "CONFIG_LOCKING_PROTECT",
            "CONFIG_PCPU_RWSEM_LOCKING_PROTECT",
        ],
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_SYSTEM_KERNEL_QCOM"],
        },
        ko_deps = [
                            "//vendor/oplus/kernel/synchronize:oplus_locking_strategy",
                            "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
                        ],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_locking_strategy",
        module_list = [
            "oplus_locking_strategy",
            "oplus_lock_torture",
        ],
    )

load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_freqqos_monitor_local_modules():

    define_oplus_ddk_module(
        name = "oplus_freqqos_monitor",
        srcs = native.glob([
            "freqqos_monitor/*.h",
            "freqqos_monitor/*.c",
        ]),
        includes = ["."],
    )

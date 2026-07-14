load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_freq_qos_arbiter_local_modules():

    define_oplus_ddk_module(
        name = "oplus_freq_qos_arbiter",
        srcs = native.glob([
            "freq_qos_arbiter/*.h",
            "freq_qos_arbiter/qos_arbiter.c",
        ]),
        includes = ["."],
    )

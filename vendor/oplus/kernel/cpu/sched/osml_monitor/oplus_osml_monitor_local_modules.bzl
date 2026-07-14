load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

def define_oplus_osml_monitor_local_modules():

    define_oplus_ddk_module(
        name = "osml_monitor",
        srcs = native.glob([
            "sched/osml_monitor/osml.h",
            "sched/osml_monitor/osml_monitor.c",
        ]),
        includes = ["."],
    )
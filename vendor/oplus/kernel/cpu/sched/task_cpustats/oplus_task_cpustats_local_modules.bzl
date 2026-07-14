load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_task_cpustats_local_modules():

    define_oplus_ddk_module(
        name = "oplus_bsp_task_cpustats",
        srcs = native.glob([
            "sched/task_cpustats/task_cpustats.c",
            "sched/task_cpustats/task_cpustats.h",
        ]),
        includes = ["."],
    )

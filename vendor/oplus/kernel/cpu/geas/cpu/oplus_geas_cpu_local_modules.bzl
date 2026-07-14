load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_geas_cpu_local_modules():
    target = oplus_ddk_get_target()
    if target == "k6993v1_64" :
        kernelconfig = "geas/cpu/kernel_config/k6993v1_64/geas_configs"
    elif target == "canoe" :
        kernelconfig = "geas/cpu/kernel_config/canoe/geas_configs"
    else :
        kernelconfig = "geas/cpu/kernel_config/default/geas_configs"

    define_oplus_ddk_module(
        name = "oplus_bsp_geas_cpu",
        srcs = native.glob([
            "geas/cpu/geas_cpu_common.h",
        ]),
        kconfig = "geas/cpu/Kconfig",
        defconfig = kernelconfig,
        conditional_srcs = {
            "CONFIG_OPLUS_FEATURE_GEAS_CPU": {
                True:[
                      "geas/cpu/geas_cpu.c",
                      "geas/cpu/geas_cpu_debug.h",
                      "geas/cpu/geas_cpu_para.c",
                      "geas/cpu/geas_cpu_para.h",
                      "geas/cpu/geas_cpu_sched.h",
                      "geas/cpu/geas_cpu_sysctrl.c",
                      "geas/cpu/geas_cpu_sysctrl.h",
                      "geas/cpu/geas_debug.c",
                      "geas/cpu/geas_dyn_em.c",
                      "geas/cpu/geas_dyn_em.h",
                      "geas/cpu/geas_task_manager.c",
                      "geas/cpu/geas_task_manager.h",
                      "geas/cpu/pipline_eas.c",
                      "geas/cpu/pipline_eas.h",
                      "geas/cpu/sharebuck.c",
                      "geas/cpu/sharebuck.h",
                      "geas/cpu/trace_geas.h",
                      "geas/cpu/geas_cpu_external.h",
                      ],
                False:["geas/cpu/empty.c"],
            },
        },
        ko_deps = [
            "//vendor/oplus/kernel/cpu:oplus_bsp_game_opt",
    ],
    )


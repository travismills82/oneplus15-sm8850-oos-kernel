load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target", "bazel_support_platform", "oplus_ddk_get_variant")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_game_opt_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)

    if bazel_support_platform == "qcom" :
        ddk_config = "//soc-repo:{}_config".format(kernel_build_variant)
        copts = []
        ko_deps = ["//soc-repo:{}/kernel/sched/walt/sched-walt".format(kernel_build_variant)]
    else :
        ddk_config = None
        copts = [
        "-DCONFIG_OPLUS_FEATURE_GEAS",
        ]
        ko_deps = []

    define_oplus_ddk_module(
        name = "oplus_bsp_game_opt",
        srcs = native.glob([
            "game_opt/**/*.h",
            "game_opt/cpu_load.c",
            "game_opt/cpufreq_limits.c",
            "game_opt/debug.c",
            "game_opt/early_detect.c",
            "game_opt/game_ctrl.c",
            "game_opt/rt_info.c",
            "game_opt/task_load_track.c",
            "game_opt/task_util.c",
            "game_opt/multi_task_util.c",
            "game_opt/multi_rt_info.c",
            "game_opt/yield_opt.c",
            "game_opt/frame_load.c",
            "game_opt/frame_sync.c",
            "game_opt/task_boost/heavy_task_boost.c",
            "game_opt/task_boost/boost_proc.c",
            "game_opt/frame_detect/frame_detect.c",
            "game_opt/oem_data/game_oem_data.c",
            "game_opt/critical_task_boost.c",
            "game_opt/geas_ctrl.c",
            "game_opt/hybrid_frame_sync.c",
            "game_opt/game_sysctl.c",
            "game_opt/rwsem_opt.c",
        ]),
        conditional_srcs = {
            "CONFIG_OPLUS_FEATURE_GEAS_CPU": {
                True:["game_opt/geas_assist.c"],
            },
        },
        includes = ["."],
        config = ddk_config,
        copts = copts,
        ko_deps = ko_deps,
        header_deps = ["//vendor/oplus/kernel/cpu:config_headers"],
        local_defines = [],
    )

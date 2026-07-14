load("//build/kernel/kleaf:kernel.bzl", "ddk_module")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//vendor/qcom/opensource/mm-drivers:target_variants.bzl", "get_all_variants")

def _define_module(target, variant):
    tv = "{}_{}".format(target, variant)

    deps = select({
        "//build/kernel/kleaf:socrepo_true": [
            "//soc-repo:all_headers",
        ],
        "//build/kernel/kleaf:socrepo_false": [
            "//msm-kernel:all_headers",
        ],
    })

    kernel_build = select({
        "//build/kernel/kleaf:socrepo_true": "//soc-repo:{}_base_kernel".format(tv),
        "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(tv),
    })

    if target in ["seraph"]:
        target_config = "seraph_defconfig"
    else:
        target_config = "defconfig"

    ddk_module(
        name = "{}_msm_hfi_core".format(tv),
        srcs = [
            "src/hfi_transport/hfi_queue.c",
            "src/hfi_transport/hfi_ipc.c",
            "src/hfi_transport/hfi_smmu.c",
            "src/hfi_transport/hfi_swi.c",
            "src/hfi_transport/hfi_queue_controller.c",
            "src/hfi_transport/hfi_if_abstraction.c",
            "src/hfi_base/hfi_core.c",
            "src/hfi_dbg_packet.c",
            "src/hfi_core_debug.c",
            "src/hfi_core_probe.c",
        ],
        out = "msm_hfi_core.ko",
        defconfig = target_config,
        kconfig = "Kconfig",
        deps = deps + [
            "//vendor/qcom/opensource/mm-drivers:mm_drivers_headers",
        ],
        kernel_build = kernel_build,
    )

    copy_to_dist_dir(
        name = "{}_msm_hfi_core_dist".format(tv),
        data = [":{}_msm_hfi_core".format(tv)],
        dist_dir = "out/target/product/{}/dlkm/lib/modules".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
        log = "info",
    )

def define_hfi_core():
    for (t, v) in get_all_variants():
        _define_module(t, v)

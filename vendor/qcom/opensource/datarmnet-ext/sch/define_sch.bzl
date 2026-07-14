load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//build/kernel/kleaf:kernel.bzl", "ddk_module")

def define_sch(target, variant):
    kernel_build_variant = "{}_{}".format(target, variant)
    include_base = "../../../{}".format(native.package_name())

    deps_sch = select({
	"//build/kernel/kleaf:socrepo_true": ["//soc-repo:all_headers"],
	"//build/kernel/kleaf:socrepo_false": ["//msm-kernel:all_headers"],
    })

    kernel_build = select({
	"//build/kernel/kleaf:socrepo_true": "//soc-repo:{}_base_kernel".format(kernel_build_variant),
	"//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(kernel_build_variant),
    })

    ddk_module(
        name = "{}_sch".format(kernel_build_variant),
        out = "rmnet_sch.ko",
        srcs = [
            "rmnet_sch_main.c",
        ],
        deps = deps_sch,
        copts = ["-Wno-misleading-indentation"],
        kernel_build = kernel_build,
    )

    copy_to_dist_dir(
        name = "{}_datarment-ext_dist".format(kernel_build_variant),
        data = [
            ":{}_sch".format(kernel_build_variant),
        ],
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
    )

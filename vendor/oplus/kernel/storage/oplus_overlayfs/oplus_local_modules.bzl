load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():

    define_oplus_ddk_module(
        name = "oplus_overlay",
        srcs = native.glob([
                "copy_up.c",
                "dir.c",
                "export.c",
                "file.c",
                "inode.c",
                "namei.c",
                "params.c",
                "readdir.c",
                "super.c",
                "util.c",
                "xattrs.c",
                "*.h",
        ]),
        includes = ["."],
        local_defines = [
            "CONFIG_OPLUS_OVERLAY_FS",
        ],
    )
    ddk_copy_to_dist_dir(
        name = "oplus_overlay",
        module_list = [
            "oplus_overlay",
        ],
    )

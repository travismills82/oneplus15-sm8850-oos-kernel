load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_kernel_version", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")
def define_oplus_storage_modules():
    kernel_version = oplus_ddk_get_kernel_version()

    # add for ufs_oplus_dbg
    if bazel_support_platform == "qcom":
        copts = [
                    "-DCONFIG_OPLUS_QCOM_UFS_DRIVER",
                    "-I$(srctree)/drivers/ufs/host/",
                ]
        ko_deps = ["//vendor/oplus/kernel/device_info/device_info/bazel:device_info"]
        hdrs = [
            "storage_feature_in_module/common/ufs_oplus_dbg/ufs-oplus-dbg.h",
            "storage_feature_in_module/common/ufs_oplus_dbg/ufs-qcom.h",
            "storage_feature_in_module/common/ufs_oplus_dbg/ufshcd-priv.h",
        ]
        header_deps = []
    else:
        copts = [
                    "-I$(srctree)/drivers/ufs/core/",
                    "-I$(DEVICE_MODULES_PATH)/drivers/ufs/",
                ]
        hdrs = [
            "storage_feature_in_module/common/ufs_oplus_dbg/ufs-oplus-dbg.h",
        ]
        ko_deps = [
                    "//kernel_device_modules-{}/drivers/soc/oplus/device_info:device_info".format(kernel_version),
        ]
        header_deps = ["//kernel_device_modules-{}/drivers/ufs:headers".format(kernel_version),]

    define_oplus_ddk_module(
        name = "ufs-oplus-dbg",
        srcs = native.glob([
            "storage_feature_in_module/common/ufs_oplus_dbg/*.c",
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        conditional_defines = {
            "qcom": ["CONFIG_OPLUS_QCOM_UFS_DRIVER"],
        },
        header_deps = header_deps,
        out = "ufs-oplus-dbg.ko",
    )

    # add for oplus_bsp_storage_io_metrics
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = []
        hdrs = ["common/io_metrics/ufs_trace.h",]
    else:
        copts = ["-I$(srctree)/drivers/ufs/core/",
                 "-I$(srctree)/include/"]
        configs = []
        ko_deps = []
        hdrs = []

    define_oplus_ddk_module(
        name = "oplus_bsp_storage_io_metrics",
        srcs = native.glob([
            "common/io_metrics/abnormal_io.c",
            "common/io_metrics/abnormal_io.h",
            "common/io_metrics/block_metrics.c",
            "common/io_metrics/block_metrics.h",
            "common/io_metrics/io_metrics_entry.c",
            "common/io_metrics/io_metrics_entry.h",
            "common/io_metrics/procfs.c",
            "common/io_metrics/procfs.h",
            "common/io_metrics/ufs_metrics.c",
            "common/io_metrics/ufs_metrics.h",
        ]),
        hdrs = hdrs,
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS"],
        copts = copts,
        ko_deps = ko_deps,
        out = "oplus_bsp_storage_io_metrics.ko",
    )

    #  add for storage_log
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = []
        hdrs = []
    else:
        copts = []
        ko_deps = []
        hdrs = ["include/storage.h",]
        configs = []

    define_oplus_ddk_module(
        name = "storage_log",
        srcs = native.glob([
            "storage_feature_in_module/common/storage_log/*.c"
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        out = "storage_log.ko",
    )

    # add for oplus_uprobe
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = ["//vendor/oplus/kernel/storage:storage_log"]
        hdrs = ["storage_feature_in_module/common/oplus_uprobe/kernel/trace/trace_probe.h",
                "storage_feature_in_module/common/oplus_uprobe/kernel/trace/trace.h",
                "storage_feature_in_module/common/oplus_uprobe/kernel/trace/pid_list.h",
                "storage_feature_in_module/common/oplus_uprobe/kernel/trace/trace_entries.h",
                "storage_feature_in_module/common/oplus_uprobe/kernel/trace/trace_output.h",
       ]
    else:
        copts = []
        ko_deps = [
            "//kernel_device_modules-{}/drivers/soc/oplus/storage:storage_log".format(kernel_version),
        ]
        hdrs = [
            "include/storage.h",
            "storage_feature_in_module/common/oplus_uprobe/oplus_uprobe.h"
        ]

    define_oplus_ddk_module(
        name = "oplus_uprobe",
        srcs = native.glob([
            "storage_feature_in_module/common/oplus_uprobe/*.c"
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        out = "oplus_uprobe.ko",
    )

    # add for oplus_file_record
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = []
        hdrs = []
    else:
        copts = []
        ko_deps = []
        hdrs = []

    define_oplus_ddk_module(
        name = "oplus_file_record",
        srcs = native.glob([
            "storage_feature_in_module/common/file_record/*.c"
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        out = "oplus_file_record.ko",
    )

    # add for oplus_f2fs_log
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = []
        hdrs = []
    else:
        copts = []
        ko_deps = [
            "//kernel_device_modules-{}/drivers/soc/oplus/storage:storage_log".format(kernel_version),
        ]
        hdrs = [
            "include/storage.h",
        ]

    define_oplus_ddk_module(
        name = "oplus_f2fslog_storage",
        srcs = native.glob([
            "storage_feature_in_module/common/oplus_f2fslog_storage/*.c"
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        out = "oplus_f2fslog_storage.ko",
    )

    # add for oplus_wq_dynamic_priority
    if bazel_support_platform == "qcom":
        copts = []
        ko_deps = [
            "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
        ]
        hdrs = [
            "storage_feature_in_module/common/wq_dynamic_priority/oplus_wq_dynamic_priority.h"
        ]
    else:
        copts = []
        ko_deps = [
            "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
        ]
        hdrs = [
            "storage_feature_in_module/common/wq_dynamic_priority/oplus_wq_dynamic_priority.h",
        ]
    define_oplus_ddk_module(
        name = "oplus_wq_dynamic_priority",
        srcs = native.glob([
            "storage_feature_in_module/common/wq_dynamic_priority/*.c"
        ]),
        hdrs = hdrs,
        includes = ["."],
        copts = copts,
        ko_deps = ko_deps,
        out = "oplus_wq_dynamic_priority.ko",
    )

    ddk_copy_to_dist_dir(
        name = "oplus_storage",
        module_list = [
            "ufs-oplus-dbg",
            "oplus_bsp_storage_io_metrics",
            "oplus_uprobe",
            "storage_log",
            "oplus_wq_dynamic_priority",
            "oplus_file_record",
        ],
    )

load("//build/kernel/kleaf:kernel.bzl", "kernel_module_group")
load(":configs/autogvm_consolidate.bzl", "autogvm_consolidate_config")
load(":configs/autogvm_perf.bzl", "autogvm_perf_config")
load(":kleaf-scripts/android_build.bzl", "define_typical_android_build")
load(":kleaf-scripts/image_opts.bzl", "boot_image_opts")
load(":kleaf-scripts/vm_build.bzl", "define_typical_vm_build")
load(":target_variants.bzl", "la_variants")

target_name = "autogvm"

def define_autogvm():
    kernel_vendor_cmdline_extras = ["bootconfig"]
    for variant in la_variants:
        board_kernel_cmdline_extras = []
        board_bootconfig_extras = []

        if variant == "consolidate":
            board_bootconfig_extras += ["androidboot.serialconsole=1"]
            board_kernel_cmdline_extras += [
                # do not sort
                "console=hvc0,115200n8",
                "qcom_geni_serial.con_enabled=1",
                "earlycon",
            ]
            kernel_vendor_cmdline_extras += [
                # do not sort
                "console=hvc0,115200n8",
                "qcom_geni_serial.con_enabled=1",
                "earlycon",
            ]

            consolidate_build_img_opts = boot_image_opts(
                boot_partition_size = 0x4000000,
                kernel_vendor_cmdline_extras = kernel_vendor_cmdline_extras,
                board_kernel_cmdline_extras = board_kernel_cmdline_extras,
                board_bootconfig_extras = board_bootconfig_extras,
            )

        else:
            board_kernel_cmdline_extras += ["nosoftlockup console=hvc0,115200n8 qcom_geni_serial.con_enabled=1 androidboot.first_stage_console=1 androidboot.bootdevice=1c170000.virtio_blk"]
            kernel_vendor_cmdline_extras += ["nosoftlockup console=hvc0,115200n8 qcom_geni_serial.con_enabled=1 androidboot.first_stage_console=1 androidboot.bootdevice=1c170000.virtio_blk"]
            board_bootconfig_extras += ["androidboot.serialconsole=0"]

            perf_build_img_opts = boot_image_opts(
                boot_partition_size = 0x4000000,
                kernel_vendor_cmdline_extras = kernel_vendor_cmdline_extras,
                board_kernel_cmdline_extras = board_kernel_cmdline_extras,
                board_bootconfig_extras = board_bootconfig_extras,
            )

    define_typical_android_build(
        name = "autogvm",
        consolidate_config = autogvm_perf_config | autogvm_consolidate_config,
        perf_config = autogvm_perf_config,
        consolidate_build_img_opts = consolidate_build_img_opts,
        perf_build_img_opts = perf_build_img_opts,
    )

def register_modules(registry):
    registry.register(
        name = "drivers/tty/hvc/hvc_gunyah",
        out = "hvc_gunyah.ko",
        config = "CONFIG_HVC_GUNYAH",
        srcs = [
            # do not sort
            "drivers/tty/hvc/hvc_gunyah.c",
        ],
        deps = [
            # do not sort
            "drivers/virt/gunyah/gunyah_loader",
            "drivers/soc/qcom/mdt_loader",
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

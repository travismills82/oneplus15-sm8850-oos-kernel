def register_modules(registry):
    registry.register(
        name = "arch/arm64/gunyah/gh_arm_drv",
        out = "gh_arm_drv.ko",
        config = "CONFIG_GH_ARM64_DRV",
        srcs = [
            # do not sort
            "arch/arm64/gunyah/gh_arm.c",
            "arch/arm64/gunyah/irq.c",
            "arch/arm64/gunyah/reset.c",
            "arch/arm64/gunyah/reset.h",
        ],
    )

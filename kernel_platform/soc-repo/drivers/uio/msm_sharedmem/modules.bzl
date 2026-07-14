def register_modules(registry):
    registry.register(
        name = "drivers/uio/msm_sharedmem/msm_sharedmem",
        out = "msm_sharedmem.ko",
        config = "CONFIG_UIO_MSM_SHAREDMEM",
        srcs = [
            # do not sort
            "drivers/uio/msm_sharedmem/msm_sharedmem.c",
        ],
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

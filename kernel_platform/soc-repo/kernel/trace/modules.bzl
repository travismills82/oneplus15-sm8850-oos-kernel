def register_modules(registry):
    registry.register(
        name = "kernel/trace/qcom_ipc_logging",
        out = "qcom_ipc_logging.ko",
        config = "CONFIG_IPC_LOGGING",
        srcs = [
            # do not sort
            "kernel/trace/ipc_logging.c",
            "kernel/trace/ipc_logging_debug.c",
            "kernel/trace/ipc_logging_private.h",
        ],
        conditional_srcs = {
            "CONFIG_IPC_LOGGING_CDEV": {
                True: [
                    # do not sort
                    "kernel/trace/ipc_logging_cdev.c",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/soc/qcom/minidump",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/debug_symbol",
            "drivers/dma-buf/heaps/qcom_dma_heaps",
            "drivers/iommu/msm_dma_iommu_mapping",
            "drivers/soc/qcom/mem_buf/mem_buf_dev",
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

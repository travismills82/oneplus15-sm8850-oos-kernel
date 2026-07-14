def register_modules(registry):
    registry.register(
        name = "drivers/mailbox/msm_qmp",
        out = "msm_qmp.ko",
        config = "CONFIG_MSM_QMP",
        srcs = [
            # do not sort
            "drivers/mailbox/msm_qmp.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/qcom_aoss",
            "kernel/trace/qcom_ipc_logging",
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

    registry.register(
        name = "drivers/mailbox/qcom-ipcc",
        out = "qcom-ipcc.ko",
        config = "CONFIG_QCOM_IPCC",
        srcs = [
            # do not sort
            "drivers/mailbox/qcom-ipcc.c",
        ],
    )

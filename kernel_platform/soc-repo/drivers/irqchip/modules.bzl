def register_modules(registry):
    registry.register(
        name = "drivers/irqchip/msm_show_resume_irq",
        out = "msm_show_resume_irq.ko",
        config = "CONFIG_QCOM_SHOW_RESUME_IRQ",
        srcs = [
            # do not sort
            "drivers/irqchip/msm_show_resume_irq.c",
        ],
    )

    registry.register(
        name = "drivers/irqchip/qcom-pdc",
        out = "qcom-pdc.ko",
        config = "CONFIG_QCOM_PDC",
        srcs = [
            # do not sort
            "drivers/irqchip/qcom-pdc.c",
        ],
        deps = [
            # do not sort
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

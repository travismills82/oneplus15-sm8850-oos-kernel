def register_modules(registry):
    registry.register(
        name = "drivers/tty/serial/msm_geni_serial",
        out = "msm_geni_serial.ko",
        config = "CONFIG_SERIAL_MSM_GENI",
        srcs = [
            # do not sort
            "drivers/tty/serial/msm_geni_serial.c",
            "drivers/tty/serial/serial_trace.h",
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

    registry.register(
        name = "drivers/tty/serial/qcom_geni_serial_msm",
        out = "qcom_geni_serial_msm.ko",
        config = "CONFIG_SERIAL_QCOM_GENI_MSM",
        srcs = [
            # do not sort
            "drivers/tty/serial/qcom_geni_serial_msm.c",
        ],
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

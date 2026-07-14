def register_modules(registry):
    registry.register(
        name = "drivers/powercap/qcom/qptf",
        out = "qptf.ko",
        config = "CONFIG_QCOM_POWER_TELEMETRY_FRAMEWORK",
        srcs = [
            # do not sort
            "drivers/powercap/qcom/qptf.c",
            "drivers/powercap/qcom/qptf_internal.h",
        ],
    )

    registry.register(
        name = "drivers/powercap/qcom/qti_power_telemetry",
        out = "qti_power_telemetry.ko",
        config = "CONFIG_QCOM_POWER_TELEMETRY_HW",
        srcs = [
            # do not sort
            "drivers/powercap/qcom/qti_power_telemetry.c",
            "drivers/powercap/qcom/qti_power_telemetry.h",
            "drivers/powercap/qcom/trace.h",
        ],
        deps = [
            # do not sort
            "drivers/powercap/qcom/qptf",
            "drivers/nvmem/nvmem_qcom-spmi-sdam",
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
        name = "drivers/powercap/qcom/qti_ptd_share",
        out = "qti_ptd_share.ko",
        config = "CONFIG_QCOM_POWER_TELEMETRY_DATA_SHARE",
        srcs = [
            # do not sort
            "drivers/powercap/qcom/qti_ptd_share.c",
            "drivers/powercap/qcom/qti_ptd_share.h",
        ],
        deps = [
            # do not sort
            "drivers/powercap/qcom/qptf",
            "drivers/nvmem/nvmem_qcom-spmi-sdam",
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

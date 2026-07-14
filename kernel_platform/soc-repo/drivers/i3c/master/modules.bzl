def register_modules(registry):
    registry.register(
        name = "drivers/i3c/master/i3c-master-msm-geni",
        out = "i3c-master-msm-geni.ko",
        config = "CONFIG_I3C_MASTER_MSM_GENI",
        srcs = [
            # do not sort
            "drivers/i3c/master/i3c-master-msm-geni.c",
            "drivers/i3c/master/i3c-qup-trace.h",
        ],
        deps = [
            # do not sort
            "drivers/dma/qcom/msm_gpi",
            "drivers/pinctrl/qcom/pinctrl-msm",
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

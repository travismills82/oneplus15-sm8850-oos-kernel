def register_modules(registry):
    registry.register(
        name = "drivers/soc/qcom/sps/sps_drv",
        out = "sps_drv.ko",
        config = "CONFIG_SPS",
        srcs = [
            # do not sort
            "drivers/soc/qcom/sps/bam.c",
            "drivers/soc/qcom/sps/bam.h",
            "drivers/soc/qcom/sps/sps.c",
            "drivers/soc/qcom/sps/sps_bam.c",
            "drivers/soc/qcom/sps/sps_bam.h",
            "drivers/soc/qcom/sps/sps_core.h",
            "drivers/soc/qcom/sps/sps_dma.c",
            "drivers/soc/qcom/sps/sps_map.c",
            "drivers/soc/qcom/sps/sps_map.h",
            "drivers/soc/qcom/sps/sps_mem.c",
            "drivers/soc/qcom/sps/sps_rm.c",
            "drivers/soc/qcom/sps/spsi.h",
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

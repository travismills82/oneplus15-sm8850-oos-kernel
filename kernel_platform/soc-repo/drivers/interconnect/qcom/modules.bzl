def register_modules(registry):
    registry.register(
        name = "drivers/interconnect/qcom/icc-bcm-voter",
        out = "icc-bcm-voter.ko",
        config = "CONFIG_INTERCONNECT_QCOM_BCM_VOTER",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/bcm-voter.c",
            "drivers/interconnect/qcom/bcm-voter.h",
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/trace.h",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/crm-v2",
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
        name = "drivers/interconnect/qcom/icc-debug",
        out = "icc-debug.ko",
        config = "CONFIG_INTERCONNECT_QCOM_DEBUG",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/icc-debug.c",
        ],
    )

    registry.register(
        name = "drivers/interconnect/qcom/icc-rpmh",
        out = "icc-rpmh.ko",
        config = "CONFIG_INTERCONNECT_QCOM_RPMH",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/bcm-voter.h",
            "drivers/interconnect/qcom/icc-debug.h",
            "drivers/interconnect/qcom/icc-rpmh.c",
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/socinfo",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/crm-v2",
            "kernel/trace/qcom_ipc_logging",
        ],
    )

    registry.register(
        name = "drivers/interconnect/qcom/qnoc-canoe",
        out = "qnoc-canoe.ko",
        config = "CONFIG_INTERCONNECT_QCOM_CANOE",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/canoe.c",
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
        ],
        deps = [
            # do not sort
            "drivers/interconnect/qcom/qnoc-qos",
            "drivers/interconnect/qcom/icc-rpmh",
            "drivers/soc/qcom/socinfo",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/crm-v2",
            "kernel/trace/qcom_ipc_logging",
        ],
    )

    registry.register(
        name = "drivers/interconnect/qcom/qnoc-yupik",
        out = "qnoc-yupik.ko",
        config = "CONFIG_INTERCONNECT_QCOM_YUPIK",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/yupik.c",
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
        ],
        deps = [
            # do not sort
            "drivers/interconnect/qcom/qnoc-qos",
            "drivers/interconnect/qcom/icc-rpmh",
            "drivers/soc/qcom/socinfo",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/crm-v2",
            "kernel/trace/qcom_ipc_logging",
        ],
    )

    registry.register(
        name = "drivers/interconnect/qcom/qnoc-qos",
        out = "qnoc-qos.ko",
        config = "CONFIG_INTERCONNECT_QCOM_QOS",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.c",
            "drivers/interconnect/qcom/qnoc-qos.h",
        ],
    )

    registry.register(
        name = "drivers/interconnect/qcom/qnoc-sun",
        out = "qnoc-sun.ko",
        config = "CONFIG_INTERCONNECT_QCOM_SUN",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
            "drivers/interconnect/qcom/sun.c",
        ],
        deps = [
            # do not sort
            "drivers/interconnect/qcom/qnoc-qos",
            "drivers/interconnect/qcom/icc-rpmh",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/socinfo",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/crm-v2",
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
        name = "drivers/interconnect/qcom/qnoc-vienna",
        out = "qnoc-vienna.ko",
        config = "CONFIG_INTERCONNECT_QCOM_VIENNA",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
            "drivers/interconnect/qcom/vienna.c",
        ],
        deps = [
            # do not sort
            "drivers/interconnect/qcom/qnoc-qos",
            "drivers/interconnect/qcom/icc-rpmh",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/socinfo",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/crm-v2",
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
        name = "drivers/interconnect/qcom/qnoc-alor",
        out = "qnoc-alor.ko",
        config = "CONFIG_INTERCONNECT_QCOM_ALOR",
        srcs = [
            # do not sort
            "drivers/interconnect/qcom/alor.c",
            "drivers/interconnect/qcom/icc-rpmh.h",
            "drivers/interconnect/qcom/qnoc-qos.h",
        ],
        deps = [
            # do not sort
            "drivers/interconnect/qcom/qnoc-qos",
            "drivers/interconnect/qcom/icc-rpmh",
            "drivers/soc/qcom/socinfo",
            "drivers/interconnect/qcom/icc-debug",
            "drivers/interconnect/qcom/icc-bcm-voter",
            "drivers/soc/qcom/qcom_rpmh",
            "drivers/soc/qcom/cmd-db",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/crm-v2",
            "kernel/trace/qcom_ipc_logging",
        ],
    )

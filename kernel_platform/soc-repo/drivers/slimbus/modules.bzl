def register_modules(registry):
    registry.register(
        name = "drivers/slimbus/slimbus",
        out = "slimbus.ko",
        config = "CONFIG_SLIMBUS",
        srcs = [
            # do not sort
            "drivers/slimbus/core.c",
            "drivers/slimbus/messaging.c",
            "drivers/slimbus/sched.c",
            "drivers/slimbus/slimbus.h",
            "drivers/slimbus/stream.c",
        ],
    )

    registry.register(
        name = "drivers/slimbus/slim-qcom-ngd-ctrl",
        out = "slim-qcom-ngd-ctrl.ko",
        config = "CONFIG_SLIM_QCOM_NGD_CTRL",
        srcs = [
            # do not sort
            "drivers/slimbus/qcom-ngd-ctrl.c",
            "drivers/slimbus/slimbus.h",
            "drivers/slimbus/trace.h",
        ],
        deps = [
            # do not sort
            "drivers/slimbus/slimbus",
            "drivers/soc/qcom/pdr_interface",
            "drivers/soc/qcom/qmi_helpers",
            "drivers/remoteproc/rproc_qcom_common",
            "drivers/rpmsg/qcom_smd",
            "drivers/rpmsg/qcom_glink_smem",
            "drivers/rpmsg/qcom_glink",
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

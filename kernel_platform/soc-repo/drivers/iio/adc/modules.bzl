def register_modules(registry):
    registry.register(
        name = "drivers/iio/adc/qcom-spmi-adc5-gen3",
        out = "qcom-spmi-adc5-gen3.ko",
        config = "CONFIG_QCOM_SPMI_ADC5_GEN3",
        srcs = [
            # do not sort
            "drivers/iio/adc/qcom-spmi-adc5-gen3.c",
        ],
        deps = [
            # do not sort
            "drivers/iio/adc/qcom-vadc-common",
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
        name = "drivers/iio/adc/qcom-vadc-common",
        out = "qcom-vadc-common.ko",
        config = "CONFIG_QCOM_VADC_COMMON",
        srcs = [
            # do not sort
            "drivers/iio/adc/qcom-vadc-common.c",
        ],
    )

    registry.register(
        name = "drivers/iio/adc/qti-glink-adc",
        out = "qti-glink-adc.ko",
        config = "CONFIG_QTI_GLINK_ADC",
        srcs = [
            # do not sort
            "drivers/iio/adc/qti-glink-adc.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/qti_pmic_glink",
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

    registry.register(
        name = "drivers/iio/adc/qcom-spmi-adc5",
        out = "qcom-spmi-adc5.ko",
        config = "CONFIG_QCOM_SPMI_ADC5",
        srcs = [
            # do not sort
            "drivers/iio/adc/qcom-spmi-adc5.c",
        ],
        deps = [
            # do not sort
            "drivers/iio/adc/qcom-vadc-common",
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

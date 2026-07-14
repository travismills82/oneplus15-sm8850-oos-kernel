def register_modules(registry):
    registry.register(
        name = "drivers/iommu/arm/arm-smmu/arm_smmu",
        out = "arm_smmu.ko",
        config = "CONFIG_ARM_SMMU",
        srcs = [
            # do not sort
            "drivers/iommu/arm/arm-smmu/arm-smmu-impl.c",
            "drivers/iommu/arm/arm-smmu/arm-smmu-nvidia.c",
            "drivers/iommu/arm/arm-smmu/arm-smmu-trace.h",
            "drivers/iommu/arm/arm-smmu/arm-smmu.c",
            "drivers/iommu/arm/arm-smmu/arm-smmu.h",
            "drivers/iommu/iommu-logger.h",
            "drivers/iommu/qcom-dma-iommu-generic.h",
            "drivers/iommu/qcom-io-pgtable-alloc.h",
        ],
        conditional_srcs = {
            "CONFIG_ARM_SMMU_QCOM": {
                True: [
                    # do not sort
                    "drivers/iommu/arm/arm-smmu/arm-smmu-qcom.c",
                    "drivers/iommu/arm/arm-smmu/arm-smmu-qcom-pm.c",
                    "drivers/iommu/arm/arm-smmu/arm-smmu-qcom.h",
                ],
            },
            "CONFIG_ARM_SMMU_QCOM_DEBUG": {
                True: [
                    # do not sort
                    "drivers/iommu/arm/arm-smmu/arm-smmu-qcom-debug.c",
                    "drivers/iommu/arm/arm-smmu/arm-smmu-qcom.h",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/iommu/iommu-logger",
            "drivers/iommu/qcom_iommu_util",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

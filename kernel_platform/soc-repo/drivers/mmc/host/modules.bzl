def register_modules(registry):
    registry.register(
        name = "drivers/mmc/host/cqhci",
        out = "cqhci.ko",
        config = "CONFIG_MMC_CQHCI",
        srcs = [
            # do not sort
            "drivers/mmc/host/cqhci-core.c",
            "drivers/mmc/host/cqhci-crypto.h",
            "drivers/mmc/host/cqhci-crypto-qti.h",
            "drivers/mmc/host/cqhci.h",
        ],
        conditional_srcs = {
            "CONFIG_MMC_CRYPTO": {
                True: [
                    # do not sort
                    "drivers/mmc/host/cqhci-crypto.c",
                ],
            },
            "CONFIG_MMC_CRYPTO_QTI": {
                True: [
                    # do not sort
                    "drivers/mmc/host/cqhci-crypto-qti.c",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/soc/qcom/qcom_ice",
        ],
    )

    registry.register(
        name = "drivers/mmc/host/sdhci-msm",
        out = "sdhci-msm.ko",
        config = "CONFIG_MMC_SDHCI_MSM",
        srcs = [
            # do not sort
            "drivers/mmc/host/cqhci.h",
            "drivers/mmc/host/sdhci-msm.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/qcom_ice",
            "drivers/mmc/host/cqhci",
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/clk/qcom/clk-qcom",
            "drivers/clk/qcom/gdsc-regulator",
            "drivers/regulator/debug-regulator",
            "drivers/regulator/proxy-consumer",
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

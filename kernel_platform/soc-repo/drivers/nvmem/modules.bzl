def register_modules(registry):
    registry.register(
        name = "drivers/nvmem/nvmem_qcom-spmi-sdam",
        out = "nvmem_qcom-spmi-sdam.ko",
        config = "CONFIG_NVMEM_SPMI_SDAM",
        srcs = [
            # do not sort
            "drivers/nvmem/qcom-spmi-sdam.c",
        ],
    )

    registry.register(
        name = "drivers/nvmem/nvmem_qfprom",
        out = "nvmem_qfprom.ko",
        config = "CONFIG_NVMEM_QCOM_QFPROM",
        srcs = [
            # do not sort
            "drivers/nvmem/qfprom.c",
        ],
    )

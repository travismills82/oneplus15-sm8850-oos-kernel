def register_modules(registry):
    registry.register(
        name = "drivers/edac/qcom_edac",
        out = "qcom_edac.ko",
        config = "CONFIG_EDAC_QCOM",
        srcs = [
            # do not sort
            "drivers/edac/qcom_edac.c",
        ],
    )

    registry.register(
        name = "drivers/edac/kryo_arm64_edac",
        out = "kryo_arm64_edac.ko",
        config = "CONFIG_EDAC_KRYO_ARM64",
        srcs = [
            # do not sort
            "drivers/edac/kryo_arm64_edac.c",
        ],
    )

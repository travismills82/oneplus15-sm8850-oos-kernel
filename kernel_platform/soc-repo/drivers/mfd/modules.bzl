def register_modules(registry):
    registry.register(
        name = "drivers/mfd/qcom-i2c-pmic",
        out = "qcom-i2c-pmic.ko",
        config = "CONFIG_MFD_I2C_PMIC",
        srcs = [
            # do not sort
            "drivers/mfd/qcom-i2c-pmic.c",
        ],
        deps = [
            # do not sort
            "drivers/base/regmap/qti-regmap-debugfs",
        ],
    )

    registry.register(
        name = "drivers/mfd/qcom-spmi-pmic",
        out = "qcom-spmi-pmic.ko",
        config = "CONFIG_MFD_SPMI_PMIC",
        srcs = [
            # do not sort
            "drivers/mfd/qcom-spmi-pmic.c",
        ],
        deps = [
            # do not sort
            "drivers/base/regmap/qti-regmap-debugfs",
        ],
    )

    registry.register(
        name = "drivers/mfd/qcom-pm8008",
        out = "qcom-pm8008.ko",
        config = "CONFIG_MFD_QCOM_PM8008",
        srcs = [
            # do not sort
            "drivers/mfd/qcom-pm8008.c",
        ],
        deps = [
            # do not sort
        ],
    )

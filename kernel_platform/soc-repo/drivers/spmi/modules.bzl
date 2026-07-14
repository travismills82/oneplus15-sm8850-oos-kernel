def register_modules(registry):
    registry.register(
        name = "drivers/spmi/spmi-pmic-arb-debug",
        out = "spmi-pmic-arb-debug.ko",
        config = "CONFIG_SPMI_MSM_PMIC_ARB_DEBUG",
        srcs = [
            # do not sort
            "drivers/spmi/spmi-pmic-arb-debug.c",
        ],
    )

    registry.register(
        name = "drivers/spmi/spmi-pmic-arb",
        out = "spmi-pmic-arb.ko",
        config = "CONFIG_SPMI_MSM_PMIC_ARB",
        srcs = [
            # do not sort
            "drivers/spmi/spmi-pmic-arb.c",
        ],
    )

    registry.register(
        name = "drivers/spmi/viospmi-pmic-arb",
        out = "viospmi-pmic-arb.ko",
        config = "CONFIG_VIOSPMI_MSM_PMIC_ARB",
        srcs = [
            # do not sort
            "drivers/spmi/viospmi-pmic-arb.c",
        ],
    )

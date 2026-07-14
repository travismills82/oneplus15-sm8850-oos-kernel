def register_modules(registry):
    registry.register(
        name = "drivers/leds/flash/leds-qcom-flash",
        out = "leds-qcom-flash.ko",
        config = "CONFIG_LEDS_QCOM_FLASH",
        srcs = [
            # do not sort
            "drivers/leds/flash/leds-qcom-flash.c",
        ],
        deps = [
            "//vendor/oplus/kernel/charger/bazel:{target_variant}_oplus_chg_v2",
        ],
    )

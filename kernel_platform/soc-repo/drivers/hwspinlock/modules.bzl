def register_modules(registry):
    registry.register(
        name = "drivers/hwspinlock/qcom_hwspinlock",
        out = "qcom_hwspinlock.ko",
        config = "CONFIG_HWSPINLOCK_QCOM",
        srcs = [
            # do not sort
            "drivers/hwspinlock/qcom_hwspinlock.c",
        ],
    )

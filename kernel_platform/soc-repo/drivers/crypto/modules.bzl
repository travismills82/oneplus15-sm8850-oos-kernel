def register_modules(registry):
    registry.register(
        name = "drivers/crypto/qcom-rng",
        out = "qcom-rng.ko",
        config = "CONFIG_CRYPTO_DEV_QCOM_RNG",
        srcs = [
            # do not sort
            "drivers/crypto/qcom-rng.c",
        ],
    )

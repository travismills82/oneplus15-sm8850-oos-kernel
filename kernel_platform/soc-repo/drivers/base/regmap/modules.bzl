def register_modules(registry):
    registry.register(
        name = "drivers/base/regmap/qti-regmap-debugfs",
        out = "qti-regmap-debugfs.ko",
        config = "CONFIG_REGMAP_QTI_DEBUGFS",
        srcs = [
            # do not sort
            "drivers/base/regmap/qti-regmap-debugfs.c",
        ],
    )

    registry.register(
        name = "drivers/base/regmap/regmap-mmio",
        out = "regmap-mmio.ko",
        config = "CONFIG_REGMAP_MMIO",
        srcs = [
            # do not sort
            "drivers/base/regmap/regmap-mmio.c",
        ],
        deps = [
            # do not sort
        ],
    )

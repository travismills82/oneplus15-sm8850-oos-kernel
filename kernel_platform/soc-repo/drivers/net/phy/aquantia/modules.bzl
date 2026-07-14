def register_modules(registry):
    registry.register(
        name = "drivers/net/phy/aquantia",
        out = "aquantia.ko",
        config = "CONFIG_AQUANTIA_PHY",
        srcs = [
            # do not sort
            "lib/crc-itu-t.c",
            "drivers/net/phy/aquantia/aquantia_main.c",
            "drivers/net/phy/aquantia/aquantia_firmware.c",
            "drivers/net/phy/aquantia/aquantia_leds.c",
            "drivers/net/phy/aquantia/aquantia.h",
        ],
    )

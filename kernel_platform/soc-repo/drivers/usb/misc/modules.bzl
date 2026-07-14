def register_modules(registry):
    registry.register(
        name = "drivers/usb/misc/lvstest",
        out = "lvstest.ko",
        config = "CONFIG_USB_LINK_LAYER_TEST",
        srcs = [
            # do not sort
            "drivers/usb/misc/lvstest.c",
        ],
    )

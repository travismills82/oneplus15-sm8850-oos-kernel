load(":drivers/video/backlight/modules.bzl", register_backlight = "register_modules")

def register_modules(registry):
    register_backlight(registry)

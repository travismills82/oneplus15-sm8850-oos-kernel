load(":drivers/power/reset/modules.bzl", register_reset = "register_modules")
load(":drivers/power/supply/modules.bzl", register_supply = "register_modules")

def register_modules(registry):
    register_reset(registry)
#    register_supply(registry)

load(":drivers/bus/mhi/modules.bzl", register_mhi = "register_modules")

def register_modules(registry):
    register_mhi(registry)

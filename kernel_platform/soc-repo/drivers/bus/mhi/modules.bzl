load(":drivers/bus/mhi/devices/modules.bzl", register_devices = "register_modules")
load(":drivers/bus/mhi/host/modules.bzl", register_host = "register_modules")

def register_modules(registry):
    register_devices(registry)
    register_host(registry)

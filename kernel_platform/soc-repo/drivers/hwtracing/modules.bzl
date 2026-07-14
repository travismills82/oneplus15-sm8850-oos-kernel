load(":drivers/hwtracing/coresight/modules.bzl", register_coresight = "register_modules")
load(":drivers/hwtracing/stm/modules.bzl", register_stm = "register_modules")

def register_modules(registry):
    register_coresight(registry)
    register_stm(registry)

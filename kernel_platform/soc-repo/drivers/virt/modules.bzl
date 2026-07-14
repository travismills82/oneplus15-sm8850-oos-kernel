load(":drivers/virt/gunyah/modules.bzl", register_gunyah = "register_modules")

def register_modules(registry):
    register_gunyah(registry)

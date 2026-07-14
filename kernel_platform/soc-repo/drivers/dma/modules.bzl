load(":drivers/dma/qcom/modules.bzl", register_qcom = "register_modules")

def register_modules(registry):
    register_qcom(registry)

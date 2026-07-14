load(":drivers/firmware/arm_scmi/modules.bzl", register_arm_scmi = "register_modules")
load(":drivers/firmware/qcom/modules.bzl", register_qcom = "register_modules")

def register_modules(registry):
    register_arm_scmi(registry)
    register_qcom(registry)

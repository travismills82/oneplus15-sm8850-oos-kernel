def register_modules(registry):
    registry.register(
        name = "drivers/firmware/arm_scmi/qcom_scmi_vendor",
        out = "qcom_scmi_vendor.ko",
        config = "CONFIG_QTI_SCMI_VENDOR_PROTOCOL",
        srcs = [
            # do not sort
            "drivers/firmware/arm_scmi/qcom_scmi_vendor.c",
        ],
    )

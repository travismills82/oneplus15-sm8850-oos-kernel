def register_modules(registry):
    registry.register(
        name = "drivers/soc/qcom/qpace/qpace_drv",
        out = "qpace_drv.ko",
        config = "CONFIG_QTI_PAGE_COMPRESSION_ENGINE",
        srcs = [
            # do not sort
            "drivers/soc/qcom/qpace/qpace.c",
            "drivers/soc/qcom/qpace/qpace.h",
            "drivers/soc/qcom/qpace/qpace-constants.h",
            "drivers/soc/qcom/qpace/qpace-reg-accessors.h",
        ],
    )

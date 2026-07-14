load(":drivers/clk/qcom/modules.bzl", register_qcom = "register_modules")

def register_modules(registry):
    register_qcom(registry)

    registry.register(
        name = "drivers/clk/clk-scmi",
        out = "clk-scmi.ko",
        config = "CONFIG_COMMON_CLK_SCMI",
        srcs = [
            # do not sort
            "drivers/clk/clk-scmi.c",
        ],
    )

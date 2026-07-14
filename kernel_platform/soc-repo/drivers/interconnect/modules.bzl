def register_modules(registry):
    registry.register(
        name = "drivers/interconnect/icc-clk",
        out = "icc-clk.ko",
        config = "CONFIG_INTERCONNECT_CLK",
        srcs = [
            # do not sort
            "drivers/interconnect/icc-clk.c",
        ],
        deps = [
            # do not sort
        ],
    )

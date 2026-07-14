def register_modules(registry):
    registry.register(
        name = "mm/zsmalloc",
        out = "zsmalloc.ko",
        config = "CONFIG_ZSMALLOC",
        srcs = [
            # do not sort
            "mm/zsmalloc.c",
        ],
    )

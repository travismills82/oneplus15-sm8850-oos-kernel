def register_modules(registry):
    registry.register(
        name = "drivers/gpu/drm/bridge/lt9611uxc",
        out = "lt9611uxc.ko",
        config = "CONFIG_DRM_LT9611UXC",
        srcs = [
            # do not sort
            "drivers/gpu/drm/bridge/lt9611uxc.c",
        ],
    )

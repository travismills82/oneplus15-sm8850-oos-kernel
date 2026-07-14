def register_modules(registry):
    registry.register(
        name = "drivers/char/rdbg",
        out = "rdbg.ko",
        config = "CONFIG_MSM_RDBG",
        srcs = [
            # do not sort
            "drivers/char/rdbg.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/smem",
        ],
    )

    registry.register(
        name = "drivers/char/virtio_console",
        out = "virtio_console.ko",
        config = "CONFIG_VIRTIO_CONSOLE",
        srcs = [
            # do not sort
            "drivers/char/virtio_console.c",
        ],
        deps = [
            # do not sort
            "drivers/virtio/virtio_mmio",
        ],
    )

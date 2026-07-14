def register_modules(registry):
    registry.register(
        name = "kernel/locking/locktorture",
        out = "locktorture.ko",
        config = "CONFIG_LOCK_TORTURE_TEST",
        srcs = [
            # do not sort
            "kernel/locking/locktorture.c",
        ],
        deps = [
            # do not sort
            "kernel/torture",
        ],
    )

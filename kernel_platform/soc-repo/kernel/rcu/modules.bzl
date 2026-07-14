def register_modules(registry):
    registry.register(
        name = "kernel/rcu/rcutorture",
        out = "rcutorture.ko",
        config = "CONFIG_RCU_TORTURE_TEST",
        srcs = [
            # do not sort
            "kernel/rcu/rcutorture.c",
        ],
        deps = [
            # do not sort
            "kernel/torture",
        ],
    )

def register_modules(registry):
    registry.register(
        name = "drivers/hwtracing/stm/stm_console",
        out = "stm_console.ko",
        config = "CONFIG_STM_SOURCE_CONSOLE",
        srcs = [
            # do not sort
            "drivers/hwtracing/stm/console.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/stm/stm_core",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/stm/stm_core",
        out = "stm_core.ko",
        config = "CONFIG_STM",
        srcs = [
            # do not sort
            "drivers/hwtracing/stm/core.c",
            "drivers/hwtracing/stm/policy.c",
            "drivers/hwtracing/stm/stm.h",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/stm/stm_ftrace",
        out = "stm_ftrace.ko",
        config = "CONFIG_STM_SOURCE_FTRACE",
        srcs = [
            # do not sort
            "drivers/hwtracing/stm/ftrace.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/stm/stm_core",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/stm/stm_heartbeat",
        out = "stm_heartbeat.ko",
        config = "CONFIG_STM_SOURCE_HEARTBEAT",
        srcs = [
            # do not sort
            "drivers/hwtracing/stm/heartbeat.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/stm/stm_core",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/stm/stm_p_ost",
        out = "stm_p_ost.ko",
        config = "CONFIG_STM_PROTO_OST",
        srcs = [
            # do not sort
            "drivers/hwtracing/stm/p_ost.c",
            "drivers/hwtracing/stm/stm.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/stm/stm_core",
        ],
    )

def register_modules(registry):
    registry.register(
        name = "lib/atomic64_test",
        out = "atomic64_test.ko",
        config = "CONFIG_ATOMIC64_SELFTEST",
        srcs = [
            # do not sort
            "lib/atomic64_test.c",
        ],
    )

    registry.register(
        name = "lib/test_user_copy",
        out = "test_user_copy.ko",
        config = "CONFIG_TEST_USER_COPY",
        srcs = [
            # do not sort
            "lib/test_user_copy.c",
        ],
    )

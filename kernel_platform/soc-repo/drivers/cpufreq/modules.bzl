def register_modules(registry):
    registry.register(
        name = "drivers/cpufreq/qcom-cpufreq-thermal",
        out = "qcom-cpufreq-thermal.ko",
        config = "CONFIG_ARM_QCOM_CPUFREQ_THERMAL",
        srcs = [
            # do not sort
            "drivers/cpufreq/qcom-cpufreq-thermal.c",
        ],
    )

    registry.register(
        name = "drivers/cpufreq/qcom-cpufreq-hw",
        out = "qcom-cpufreq-hw.ko",
        config = "CONFIG_ARM_QCOM_CPUFREQ_HW",
        srcs = [
            # do not sort
            "drivers/cpufreq/qcom-cpufreq-hw.c",
        ],
    )

    registry.register(
        name = "drivers/cpufreq/qcom-cpufreq-hw-debug",
        out = "qcom-cpufreq-hw-debug.ko",
        config = "CONFIG_ARM_QCOM_CPUFREQ_HW_DEBUG",
        srcs = [
            # do not sort
            "drivers/cpufreq/qcom-cpufreq-hw-debug.c",
        ],
        deps = [
            # do not sort
            "drivers/cpufreq/qcom-cpufreq-hw",
        ],
    )

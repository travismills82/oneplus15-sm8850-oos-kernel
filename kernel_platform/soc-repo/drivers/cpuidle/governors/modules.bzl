def register_modules(registry):
    registry.register(
        name = "drivers/cpuidle/governors/qcom_lpm",
        out = "qcom_lpm.ko",
        config = "CONFIG_CPU_IDLE_GOV_QCOM_LPM",
        srcs = [
            # do not sort
            "drivers/cpuidle/governors/qcom-cluster-lpm.c",
            "drivers/cpuidle/governors/qcom-lpm-sysfs.c",
            "drivers/cpuidle/governors/qcom-lpm.c",
            "drivers/cpuidle/governors/qcom-lpm.h",
            "drivers/cpuidle/governors/trace-cluster-lpm.h",
            "drivers/cpuidle/governors/trace-qcom-lpm.h",
        ],
        deps = [
            # do not sort
            "kernel/sched/walt/sched-walt",
            "drivers/soc/qcom/socinfo",
            "drivers/soc/qcom/smem",
        ],
    )

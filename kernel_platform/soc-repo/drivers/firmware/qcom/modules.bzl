load(":drivers/firmware/qcom/si_core/modules.bzl", register_si_core = "register_modules")

def register_modules(registry):
    register_si_core(registry)

    registry.register(
        name = "drivers/firmware/qcom/qcom-scm",
        out = "qcom-scm.ko",
        config = "CONFIG_QCOM_SCM",
        srcs = [
            # do not sort
            "drivers/firmware/qcom/qcom_scm-legacy.c",
            "drivers/firmware/qcom/qcom_scm-smc.c",
            "drivers/firmware/qcom/qcom_scm.c",
            "drivers/firmware/qcom/qcom_tzmem.h",
            "drivers/firmware/qcom/qcom_scm.h",
            "drivers/firmware/qcom/qtee_shmbridge_internal.h",
            "drivers/firmware/qcom/lcp-ppddr-internal.h",
        ],
        hdrs = [
            "include/linux/firmware/qcom/qcom_scm.h",
        ],
        includes = ["include/linux"],
        conditional_srcs = {
            "CONFIG_QTEE_SHM_BRIDGE": {
                True: [
                    # do not sort
                    "drivers/firmware/qcom/qtee_shmbridge.c",
                ],
            },
            "CONFIG_MSM_HAB": {
                True: [
                    # do not sort
                    "drivers/firmware/qcom/qcom_scm_hab.c",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom_tzmem",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/soc/qcom/hab/msm_hab",
        ],
    )

    registry.register(
        name = "drivers/firmware/qcom/qcom_tzmem",
        out = "qcom_tzmem.ko",
        config = "CONFIG_QCOM_TZMEM",
        srcs = [
            # do not sort
            "drivers/firmware/qcom/qcom_tzmem.c",
            "drivers/firmware/qcom/qcom_tzmem.h",
        ],
    )

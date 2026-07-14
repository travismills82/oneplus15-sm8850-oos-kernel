def register_modules(registry):
    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-canoe",
        out = "pinctrl-canoe.ko",
        config = "CONFIG_PINCTRL_CANOE",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-canoe.c",
            "drivers/pinctrl/qcom/pinctrl-msm.h",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-msm",
        out = "pinctrl-msm.ko",
        config = "CONFIG_PINCTRL_MSM",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.c",
            "drivers/pinctrl/qcom/pinctrl-msm.h",
        ],
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-spmi-gpio",
        out = "pinctrl-spmi-gpio.ko",
        config = "CONFIG_PINCTRL_QCOM_SPMI_PMIC",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-spmi-gpio.c",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-spmi-mpp",
        out = "pinctrl-spmi-mpp.ko",
        config = "CONFIG_PINCTRL_QCOM_SPMI_PMIC",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-spmi-mpp.c",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-sun",
        out = "pinctrl-sun.ko",
        config = "CONFIG_PINCTRL_SUN",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-sun.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-vienna",
        out = "pinctrl-vienna.ko",
        config = "CONFIG_PINCTRL_VIENNA",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-vienna.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-alor",
        out = "pinctrl-alor.ko",
        config = "CONFIG_PINCTRL_ALOR",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-alor.c",
            "drivers/pinctrl/qcom/pinctrl-msm.h",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-chora",
        out = "pinctrl-chora.ko",
        config = "CONFIG_PINCTRL_CHORA",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-chora.c",
            "drivers/pinctrl/qcom/pinctrl-msm.h",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-yupik",
        out = "pinctrl-yupik.ko",
        config = "CONFIG_PINCTRL_YUPIK",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-yupik.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-direwolf",
        out = "pinctrl-direwolf.ko",
        config = "CONFIG_PINCTRL_DIREWOLF",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-direwolf.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-lemans",
        out = "pinctrl-lemans.ko",
        config = "CONFIG_PINCTRL_LEMANS",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-lemans.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-x1e80100",
        out = "pinctrl-x1e80100.ko",
        config = "CONFIG_PINCTRL_X1E80100",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-x1e80100.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-monaco_auto",
        out = "pinctrl-monaco_auto.ko",
        config = "CONFIG_PINCTRL_MONACO_AUTO",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-monaco_auto.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/pinctrl/qcom/pinctrl-sa8797p",
        out = "pinctrl-sa8797p.ko",
        config = "CONFIG_PINCTRL_SA8797P",
        srcs = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm.h",
            "drivers/pinctrl/qcom/pinctrl-sa8797p.c",
        ],
        deps = [
            # do not sort
            "drivers/pinctrl/qcom/pinctrl-msm",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

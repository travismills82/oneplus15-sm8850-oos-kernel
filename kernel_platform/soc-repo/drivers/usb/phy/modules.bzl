def register_modules(registry):
    registry.register(
        name = "drivers/usb/phy/phy-generic",
        out = "phy-generic.ko",
        config = "CONFIG_NOP_USB_XCEIV",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-generic.c",
            "drivers/usb/phy/phy-generic.h",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-m31-eusb2",
        out = "phy-msm-m31-eusb2.ko",
        config = "CONFIG_USB_M31_MSM_EUSB2_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-m31-eusb2.c",
        ],
        deps = [
            # do not sort
            "drivers/usb/repeater/repeater",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-ssusb-qmp",
        out = "phy-msm-ssusb-qmp.ko",
        config = "CONFIG_USB_MSM_SSPHY_QMP",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-ssusb-qmp.c",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-qcom-emu",
        out = "phy-qcom-emu.ko",
        config = "CONFIG_USB_QCOM_EMU_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-qcom-emu.c",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-snps-hs",
        out = "phy-msm-snps-hs.ko",
        config = "CONFIG_MSM_HSUSB_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-snps-hs.c",
        ],
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom-scm",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-qusb",
        out = "phy-msm-qusb.ko",
        config = "CONFIG_MSM_QUSB_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-qusb.c",
        ],
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom-scm",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-qusb-v2",
        out = "phy-msm-qusb-v2.ko",
        config = "CONFIG_MSM_QUSB_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-qusb-v2.c",
        ],
        deps = [
            # do not sort
            "drivers/firmware/qcom/qcom-scm",
        ],
    )

    registry.register(
        name = "drivers/usb/phy/phy-msm-snps-eusb2",
        out = "phy-msm-snps-eusb2.ko",
        config = "CONFIG_USB_MSM_EUSB2_PHY",
        srcs = [
            # do not sort
            "drivers/usb/phy/phy-msm-snps-eusb2.c",
        ],
        deps = [
            # do not sort
            "drivers/usb/repeater/repeater",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

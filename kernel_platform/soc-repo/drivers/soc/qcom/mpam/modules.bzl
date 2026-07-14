def register_modules(registry):
    registry.register(
        name = "drivers/soc/qcom/mpam/mpam",
        out = "mpam.ko",
        config = "CONFIG_QTI_MPAM",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam.c",
            "drivers/soc/qcom/mpam/mpam_rpmsg.c",
            "drivers/soc/qcom/mpam/mpam_dsp.h",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/dcvs/qcom_scmi_client",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mpam/cpu_mpam",
        out = "cpu_mpam.ko",
        config = "CONFIG_QTI_CPU_MPAM_INTERFACE",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/cpu_mpam.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mpam/platform_mpam",
        out = "platform_mpam.ko",
        config = "CONFIG_QTI_PLATFORM_MPAM_INTERFACE",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/platform_mpam.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mpam/mpam_msc",
        out = "mpam_msc.ko",
        config = "CONFIG_QTI_MPAM_MSC",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam_msc.c",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mpam/mpam_msc_slc",
        out = "mpam_msc_slc.ko",
        config = "CONFIG_QTI_MPAM_MSC_SLC",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam_msc_slc.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/dcvs/qcom_scmi_client",
            "drivers/soc/qcom/mpam/mpam_msc",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mpam/slc_mpam",
        out = "slc_mpam.ko",
        config = "CONFIG_QTI_SLC_MPAM_INTERFACE",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mpam/slc_mpam.c",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/mpam/mpam_msc",
            "drivers/soc/qcom/mpam/platform_mpam",
        ],
    )

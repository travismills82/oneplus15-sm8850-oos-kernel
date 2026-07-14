def register_modules(registry):
    registry.register(
        name = "drivers/hwtracing/coresight/coresight-csr",
        out = "coresight-csr.ko",
        config = "CONFIG_CORESIGHT_CSR",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-csr.c",
            "drivers/hwtracing/coresight/coresight-priv.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-cti",
        out = "coresight-cti.ko",
        config = "CONFIG_CORESIGHT_CTI",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-cti-core.c",
            "drivers/hwtracing/coresight/coresight-cti-platform.c",
            "drivers/hwtracing/coresight/coresight-cti-sysfs.c",
            "drivers/hwtracing/coresight/coresight-cti.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-dummy",
        out = "coresight-dummy.ko",
        config = "CONFIG_CORESIGHT_DUMMY",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-dummy.c",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-qmi.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-qmi",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
            "drivers/soc/qcom/qmi_helpers",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-etm4x",
        out = "coresight-etm4x.ko",
        config = "CONFIG_CORESIGHT_SOURCE_ETM4X",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-config.h",
            "drivers/hwtracing/coresight/coresight-etm-perf.h",
            "drivers/hwtracing/coresight/coresight-etm4x-cfg.c",
            "drivers/hwtracing/coresight/coresight-etm4x-cfg.h",
            "drivers/hwtracing/coresight/coresight-etm4x-core.c",
            "drivers/hwtracing/coresight/coresight-etm4x-sysfs.c",
            "drivers/hwtracing/coresight/coresight-etm4x.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-self-hosted-trace.h",
            "drivers/hwtracing/coresight/coresight-syscfg.h",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-funnel",
        out = "coresight-funnel.ko",
        config = "CONFIG_CORESIGHT_LINKS_AND_SINKS",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-funnel.c",
            "drivers/hwtracing/coresight/coresight-priv.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight",
        out = "coresight.ko",
        config = "CONFIG_CORESIGHT",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-cfg-afdo.c",
            "drivers/hwtracing/coresight/coresight-cfg-preload.c",
            "drivers/hwtracing/coresight/coresight-cfg-preload.h",
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-config.c",
            "drivers/hwtracing/coresight/coresight-config.h",
            "drivers/hwtracing/coresight/coresight-core.c",
            "drivers/hwtracing/coresight/coresight-etm-perf.c",
            "drivers/hwtracing/coresight/coresight-etm-perf.h",
            "drivers/hwtracing/coresight/coresight-etm4x-cfg.h",
            "drivers/hwtracing/coresight/coresight-etm4x.h",
            "drivers/hwtracing/coresight/coresight-platform.c",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-syscfg-configfs.c",
            "drivers/hwtracing/coresight/coresight-syscfg-configfs.h",
            "drivers/hwtracing/coresight/coresight-syscfg.c",
            "drivers/hwtracing/coresight/coresight-syscfg.h",
            "drivers/hwtracing/coresight/coresight-sysfs.c",
            "drivers/hwtracing/coresight/coresight-trace-id.c",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-qmi",
        out = "coresight-qmi.ko",
        config = "CONFIG_CORESIGHT_QMI",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-qmi.c",
            "drivers/hwtracing/coresight/coresight-qmi.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
            "drivers/soc/qcom/qmi_helpers",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-remote-etm",
        out = "coresight-remote-etm.ko",
        config = "CONFIG_CORESIGHT_REMOTE_ETM",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-qmi.h",
            "drivers/hwtracing/coresight/coresight-remote-etm.c",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-qmi",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
            "drivers/soc/qcom/qmi_helpers",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-replicator",
        out = "coresight-replicator.ko",
        config = "CONFIG_CORESIGHT_LINKS_AND_SINKS",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-replicator.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-stm",
        out = "coresight-stm.ko",
        config = "CONFIG_CORESIGHT_STM",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-stm.c",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
            "drivers/hwtracing/stm/stm.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/stm/stm_core",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
        ],
        copts = [
            "-D__DISABLE_TRACE_MMIO__",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-tgu",
        out = "coresight-tgu.ko",
        config = "CONFIG_CORESIGHT_TGU",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-tgu.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-tmc",
        out = "coresight-tmc.ko",
        config = "CONFIG_CORESIGHT_LINK_AND_SINK_TMC",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-catu.h",
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-byte-cntr.c",
            "drivers/hwtracing/coresight/coresight-byte-cntr.h",
            "drivers/hwtracing/coresight/coresight-etm-perf.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-tmc-core.c",
            "drivers/hwtracing/coresight/coresight-tmc-etf.c",
            "drivers/hwtracing/coresight/coresight-tmc-etr.c",
            "drivers/hwtracing/coresight/coresight-tmc-usb.h",
            "drivers/hwtracing/coresight/coresight-tmc-usb.c",
            "drivers/hwtracing/coresight/coresight-tmc.h",
        ],
        deps = [
            # do not sort
            "drivers/usb/gadget/function/usb_f_qdss",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
            "drivers/usb/dwc3/dwc3-msm",
            "drivers/soc/qcom/wcd_usbss_i2c",
            "drivers/usb/typec/ucsi/ucsi_qti_glink",
            "drivers/usb/redriver/redriver",
            "drivers/usb/repeater/repeater",
            "drivers/base/regmap/qti-regmap-debugfs",
            "drivers/iommu/qcom_iommu_util",
            "drivers/soc/qcom/qti_pmic_glink",
            "drivers/soc/qcom/pdr_interface",
            "drivers/soc/qcom/qmi_helpers",
            "drivers/remoteproc/rproc_qcom_common",
            "drivers/rpmsg/qcom_smd",
            "drivers/rpmsg/qcom_glink_smem",
            "drivers/rpmsg/qcom_glink",
            "drivers/clk/qcom/clk-qcom",
            "drivers/clk/qcom/gdsc-regulator",
            "drivers/regulator/debug-regulator",
            "drivers/regulator/proxy-consumer",
            "drivers/soc/qcom/crm-v2",
            "kernel/trace/qcom_ipc_logging",
            "drivers/soc/qcom/minidump",
            "drivers/soc/qcom/smem",
            "drivers/soc/qcom/debug_symbol",
            "drivers/dma-buf/heaps/qcom_dma_heaps",
            "drivers/iommu/msm_dma_iommu_mapping",
            "drivers/soc/qcom/mem_buf/mem_buf_dev",
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-tmc-sec",
        out = "coresight-tmc-sec.ko",
        config = "CONFIG_CORESIGHT_REMOTE_ETM",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-qmi.h",
            "drivers/hwtracing/coresight/coresight-tmc-sec.c",
            "drivers/hwtracing/coresight/coresight-tmc.h",
            "drivers/hwtracing/coresight/coresight-byte-cntr.h",
            "drivers/hwtracing/coresight/coresight-tmc-usb.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-qmi",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
            "drivers/soc/qcom/qmi_helpers",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-tpda",
        out = "coresight-tpda.ko",
        config = "CONFIG_CORESIGHT_TPDA",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-tpda.c",
            "drivers/hwtracing/coresight/coresight-tpda.h",
            "drivers/hwtracing/coresight/coresight-tpdm.h",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-tpdm",
        out = "coresight-tpdm.ko",
        config = "CONFIG_CORESIGHT_TPDM",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-tpda.h",
            "drivers/hwtracing/coresight/coresight-trace-noc.h",
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-tpdm.c",
            "drivers/hwtracing/coresight/coresight-tpdm.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/hwtracing/coresight/coresight",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-trace-noc",
        out = "coresight-trace-noc.ko",
        config = "CONFIG_CORESIGHT_TRACE_NOC",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
            "drivers/hwtracing/coresight/coresight-trace-noc.c",
            "drivers/hwtracing/coresight/coresight-trace-noc.h",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
        ],
    )

    registry.register(
        name = "drivers/hwtracing/coresight/coresight-uetm",
        out = "coresight-uetm.ko",
        config = "CONFIG_CORESIGHT_UETM",
        srcs = [
            # do not sort
            "drivers/hwtracing/coresight/coresight-common.h",
            "drivers/hwtracing/coresight/coresight-priv.h",
            "drivers/hwtracing/coresight/coresight-trace-id.h",
            "drivers/hwtracing/coresight/coresight-trace-noc.h",
            "drivers/hwtracing/coresight/coresight-uetm.c",
        ],
        deps = [
            # do not sort
            "drivers/hwtracing/coresight/coresight",
            "drivers/hwtracing/coresight/coresight-csr",
            "drivers/soc/qcom/dcvs/qcom_scmi_client",
        ],
    )

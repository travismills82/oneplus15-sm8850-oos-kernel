load(":securemsm_kernel.bzl", "define_consolidate_gki_modules")

def define_vienna():
    define_consolidate_gki_modules(
        target = "vienna",
        modules = [
            "smcinvoke_dlkm",
            "tz_log_dlkm",
            "qseecom_dlkm",
            "hdcp_qseecom_dlkm",
            "qce50_dlkm",
            "qcedev-mod_dlkm",
            "qrng_dlkm",
            "qcrypto-msm_dlkm",
            "smmu_proxy_dlkm",
            "tmecom-intf_dlkm",
            "seccam_test_driver",
            "hdcp2p2_test",
            "si_core_test",
            "tornado_mod",
         ],
         extra_options = [
            "CONFIG_QCOM_SMCINVOKE",
            "CONFIG_QSEECOM_COMPAT",
            "CONFIG_QCOM_SI_CORE",
            "CONFIG_MSM_TMECOM_QMP",
            "CONFIG_QCOM_SI_CORE_TEST",
         ],
     )

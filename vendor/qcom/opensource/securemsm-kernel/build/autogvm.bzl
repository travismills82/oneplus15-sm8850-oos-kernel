load(":securemsm_kernel.bzl", "define_consolidate_gki_modules")

def define_autogvm():
    define_consolidate_gki_modules(
        target = "autogvm",
        modules = [
            "smcinvoke_dlkm",
            "qseecom_dlkm",
            "qcedev_fe_dlkm",
            "qrng_dlkm",
        ],
        extra_options = [
           "CONFIG_QCOM_SI_CORE_TEST",
           "CONFIG_QCOM_SMCINVOKE",
           "CONFIG_QSEECOM_COMPAT",
           "CONFIG_QCOM_SI_CORE",
           "CONFIG_QSEECOM",
           "CONFIG_QCEDEV_FE",
        ],
    )

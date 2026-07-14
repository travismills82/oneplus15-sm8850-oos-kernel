load(":securemsm_kernel.bzl", "define_vm_modules")

def define_sunvm():
    define_vm_modules(
        target = "sun-oemvm",
        modules = [
            "smcinvoke_dlkm",
            "smmu_proxy_dlkm",
        ],
        extra_options = [
            "CONFIG_QCOM_SMCINVOKE",
            "CONFIG_QTI_SMMU_PROXY",
            "CONFIG_QCOM_SI_CORE",
            "CONFIG_ARCH_QTI_VM",
        ],
    )
    define_vm_modules(
        target = "sun-tuivm",
        modules = [
            "smcinvoke_dlkm",
            "smmu_proxy_dlkm",
        ],
        extra_options = [
            "CONFIG_QCOM_SMCINVOKE",
            "CONFIG_QTI_SMMU_PROXY",
            "CONFIG_QCOM_SI_CORE",
            "CONFIG_ARCH_QTI_VM",
        ],
    )

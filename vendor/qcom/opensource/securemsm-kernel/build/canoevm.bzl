load(":securemsm_kernel.bzl", "define_vm_modules")

def define_canoevm():
    define_vm_modules(
        target = "canoe-oemvm",
        modules = [
            "smcinvoke_dlkm",
            "smmu_proxy_dlkm",
        ],
        extra_options = [
            "CONFIG_QCOM_SMCINVOKE",
            "CONFIG_QTI_SMMU_PROXY",
            "CONFIG_QCOM_SI_CORE",
            "CONFIG_ARCH_QTI_VM",
            "CONFIG_QCOM_LEGACY_ADDRESS_BUS_SIZE",
        ],
     )
    define_vm_modules(
        target = "canoe-tuivm",
        modules = [
            "smcinvoke_dlkm",
            "smmu_proxy_dlkm",
        ],
        extra_options = [
            "CONFIG_QCOM_SMCINVOKE",
            "CONFIG_QTI_SMMU_PROXY",
            "CONFIG_QCOM_SI_CORE",
            "CONFIG_ARCH_QTI_VM",
            "CONFIG_QCOM_LEGACY_ADDRESS_BUS_SIZE",
        ],
     )

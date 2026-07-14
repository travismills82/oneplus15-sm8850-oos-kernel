load(":display_modules.bzl", "display_driver_modules")
load(":display_driver_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_sun():
    for (t, v) in get_all_variants():
        if t == "sun":
            define_target_variant_modules(
                target = t,
                variant = v,
                registry = display_driver_modules,
                modules = [
                    "msm_drm",
                ],
                config_options = [
                    "CONFIG_DRM_MSM_SDE",
                    "CONFIG_SYNC_FILE",
                    "CONFIG_DRM_MSM_DSI",
                    "CONFIG_MDSS_HFI_ADAPTER",
                    "CONFIG_MDSS_HFI",
                    "CONFIG_DRM_MSM_DP",
                    "CONFIG_DRM_MSM_DP_MST",
                    "CONFIG_DSI_PARSER",
                    "CONFIG_DRM_SDE_WB",
                    "CONFIG_DRM_MSM_REGISTER_LOGGING",
                    "CONFIG_QCOM_MDSS_PLL",
                    "CONFIG_HDCP_QSEECOM",
                    "CONFIG_DRM_SDE_VM",
                    "CONFIG_QCOM_WCD939X_I2C",
                    "CONFIG_THERMAL_OF",
                    "CONFIG_MSM_MMRM",
                    "CONFIG_QTI_HW_FENCE",
                    "CONFIG_QCOM_SPEC_SYNC",
                    "CONFIG_MSM_EXT_DISPLAY",
                    "CONFIG_DRM_SDE_CESTA",
                    "CONFIG_SMMU_PROXY",
                    "CONFIG_DRM_MSM_HDMI"
                ],
            )

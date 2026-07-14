load(":display_modules.bzl", "display_driver_modules")
load(":display_driver_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_canoe():
    for (t, v) in get_all_variants():
        if t == "canoe":
            define_target_variant_modules(
                target = t,
                variant = v,
                registry = display_driver_modules,
                modules = [
                    "msm_drm",
                ],
                config_options = [
#ifdef OPLUS_FEATURE_DISPLAY
                    "OPLUS_FEATURE_DISPLAY",
                    "OPLUS_FEATURE_DISPLAY_ADFR",
                    "OPLUS_FEATURE_DISPLAY_HIGH_PRECISION",
                    "OPLUS_FEATURE_DISPLAY_TEMP_COMPENSATION",
                    "OPLUS_FEATURE_DISPLAY_ONSCREENFINGERPRINT",
                    "OPLUS_TRACKPOINT_REPORT",
                    "OPLUS_FEATURE_APDMR",
                    "OPLUS_FEATURE_AP_UIR_DIMMING",
#endif /* OPLUS_FEATURE_DISPLAY */
                    "CONFIG_DRM_MSM_SDE",
                    "CONFIG_SYNC_FILE",
                    "CONFIG_DRM_MSM_DSI",
                    "CONFIG_QTI_HFI_CORE",
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
                    "CONFIG_QCOM_SPEC_SYNC",
                    "CONFIG_MSM_EXT_DISPLAY",
                    "CONFIG_DRM_SDE_CESTA",
                    "CONFIG_QTI_HW_FENCE",
#ifdef CONFIG_PXLW_IRIS
                    "CONFIG_PXLW_IRIS",
                    "CONFIG_PXLW_IRIS7P",
#endif /* CONFIG_PXLW_IRIS */
                ],
            )

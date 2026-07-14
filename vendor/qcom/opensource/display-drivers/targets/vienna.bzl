load(":display_modules.bzl", "display_driver_modules")
load(":display_driver_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_vienna():
    for (t, v) in get_all_variants():
        if t == "vienna":
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
                    "CONFIG_QTI_HFI_CORE",
                    "CONFIG_MDSS_HFI_ADAPTER",
                    "CONFIG_MDSS_HFI",
                    "CONFIG_DRM_MSM_REGISTER_LOGGING",
                    "CONFIG_QCOM_MDSS_PLL",
                    "CONFIG_THERMAL_OF",
                ],
           )

load(":display_modules.bzl", "display_driver_modules")
load(":display_driver_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_sunvm():
    for (t, v) in get_all_variants():
        if t == "sun-tuivm" or t == "sun-oemvm":
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
                    "CONFIG_DSI_PARSER",
                    "CONFIG_QCOM_MDSS_PLL",
                    "CONFIG_DRM_MSM_REGISTER_LOGGING",
                    "CONFIG_DRM_SDE_VM",
                    "CONFIG_DRM_LOW_MSM_MEM_FOOTPRINT",
                ],
                vm_target = True,
            )

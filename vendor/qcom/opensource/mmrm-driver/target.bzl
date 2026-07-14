load(":mmrm_modules.bzl", "mmrm_driver_modules")
load(":mmrm_modules_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_target_modules():
    for target, variant in get_all_variants():
        define_target_variant_modules(
            target = target,
            variant = variant,
            registry = mmrm_driver_modules,
            modules = [
                "msm-mmrm"
            ],
        )

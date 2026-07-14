load("//build/kernel/kleaf:kernel.bzl", "ddk_module")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")

def _register_module_to_map(module_map, name, path, config_option, srcs, config_srcs, deps, config_deps):
    processed_config_srcs = {}
    nested_config = {}
    processed_config_deps = {}

    for config_src_name in config_srcs:
        config_src = config_srcs[config_src_name]

        if type(config_src) == "list":
            processed_config_srcs[config_src_name] = {True: config_src}
        else:
            processed_config_srcs[config_src_name] = config_src
        if type(config_src) == "dict":
            nested_config = config_src

            for nested_src, nest_name in nested_config.items():
                if nested_src == "True":
                    for nest_src in nest_name:
                        final_srcs = nest_name[nest_src]
                        processed_config_srcs[nest_src] = final_srcs

    for config_deps_name in config_deps:
        config_dep = config_deps[config_deps_name]
        if type(config_dep) == "list":
            processed_config_deps[config_deps_name] = {True: config_dep}
        else:
            processed_config_deps[config_deps_name] = config_dep

    module = struct(
        name = name,
        path = path,
        srcs = srcs,
        config_srcs = processed_config_srcs,
        config_option = config_option,
        deps = deps,
        config_deps = processed_config_deps,
    )

    module_map[name] = module

def _get_config_choices(map, options):
    choices = []
    for option in map:
        choices.extend(map[option].get(option in options, []))
    return choices

def _get_kernel_build_options(modules, config_options):
    all_options = {option: True for option in config_options}
    all_options = all_options | {module.config_option: True for module in modules if module.config_option}
    return all_options

def _get_kernel_build_module_srcs(module, options, formatter):
    srcs = module.srcs + _get_config_choices(module.config_srcs, options)
    module_path = "{}/".format(module.path) if module.path else ""
    return ["{}{}".format(module_path, formatter(src)) for src in srcs]

def _get_kernel_build_module_deps(module, options, formatter):
    deps = module.deps + _get_config_choices(module.config_deps, options)
    deps = [formatter(dep) for dep in deps]
    return deps

def display_module_entry(hdrs = []):
    module_map = {}

    def register(name, path = None, config_option = None, srcs = [], config_srcs = {}, deps = [], config_deps = {}):
        _register_module_to_map(module_map, name, path, config_option, srcs, config_srcs, deps, config_deps)

    return struct(
        register = register,
        get = module_map.get,
        hdrs = hdrs,
        module_map = module_map,
    )

def define_target_variant_modules(target, variant, registry, modules, config_options = [], vm_target = False):
    kernel_build_tv = "{}_{}".format(target, variant)
    deps = select({
            "//build/kernel/kleaf:socrepo_true": [
            "//soc-repo:all_headers",
            "//soc-repo:{}/drivers/firmware/qcom/qcom-scm".format(kernel_build_tv),
            "//soc-repo:{}/drivers/pinctrl/qcom/pinctrl-msm".format(kernel_build_tv),
            "//soc-repo:{}/drivers/clk/qcom/clk-qcom".format(kernel_build_tv),
            "//soc-repo:{}/drivers/iommu/qcom_iommu_util".format(kernel_build_tv),
            "//soc-repo:{}/drivers/virt/gunyah/gh_irq_lend".format(kernel_build_tv),
            "//soc-repo:{}/drivers/virt/gunyah/gh_rm_drv".format(kernel_build_tv),
            "//soc-repo:{}/drivers/virt/gunyah/gh_mem_notifier".format(kernel_build_tv),
            "//soc-repo:{}/drivers/virt/gunyah/gh_msgq".format(kernel_build_tv),
            "//soc-repo:{}/drivers/spmi/spmi-pmic-arb".format(kernel_build_tv),
            "//soc-repo:{}/drivers/soc/qcom/mem_buf/mem_buf_dev".format(kernel_build_tv),
            "//soc-repo:{}/drivers/iommu/msm_dma_iommu_mapping".format(kernel_build_tv),
            "//soc-repo:{}/drivers/soc/qcom/socinfo".format(kernel_build_tv),
            "//soc-repo:{}/drivers/soc/qcom/panel_event_notifier".format(kernel_build_tv),
        ],
        "//build/kernel/kleaf:socrepo_false": ["//msm-kernel:all_headers"],
        })

    if not vm_target:
        deps += select({
           "//build/kernel/kleaf:socrepo_true": ["//soc-repo:{}/drivers/soc/qcom/qcom_va_minidump".format(kernel_build_tv)],
           "//build/kernel/kleaf:socrepo_false": [],
        })

        deps += [
            "//vendor/qcom/opensource/mm-drivers:mm_drivers_headers",
        ]
        deps += select({
            "//build/kernel/kleaf:socrepo_true": [
                "//soc-repo:{}/drivers/gpu/drm/display/drm_display_helper".format(kernel_build_tv),
                "//soc-repo:{}/drivers/soc/qcom/crm-v2".format(kernel_build_tv),
                "//soc-repo:{}/drivers/soc/qcom/llcc-qcom".format(kernel_build_tv),
                "//soc-repo:{}/drivers/soc/qcom/altmode-glink".format(kernel_build_tv),
                "//soc-repo:{}/kernel/trace/qcom_ipc_logging".format(kernel_build_tv),
                "//soc-repo:{}/drivers/usb/dwc3/dwc3-msm".format(kernel_build_tv),
                "//soc-repo:{}/drivers/soc/qcom/wcd_usbss_i2c".format(kernel_build_tv),
            ],
            "//build/kernel/kleaf:socrepo_false": [],
        })

    kernel_build = select({
        "//build/kernel/kleaf:socrepo_true": "//soc-repo:{}_base_kernel".format(kernel_build_tv),
        "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(kernel_build_tv),
    })
    modules = [registry.get(module_name) for module_name in modules]
    options = _get_kernel_build_options(modules, config_options)
    build_print = lambda message: print("{}: {}".format(kernel_build_tv, message))
    formatter = lambda s: s.replace("%b", kernel_build_tv).replace("%t", target)

    headers = deps + registry.hdrs
    all_module_rules = []

    for module in modules:
        rule_name = "{}_{}".format(kernel_build_tv, module.name)
        module_srcs = _get_kernel_build_module_srcs(module, options, formatter)
        print(rule_name)
        if not module_srcs:
            continue

        ddk_module(
            name = rule_name,
            srcs = module_srcs,
            kernel_build = kernel_build,
            out = "{}.ko".format(module.name),
            deps = headers + _get_kernel_build_module_deps(module, options, formatter),
            local_defines = options.keys(),
        )
        all_module_rules.append(rule_name)

    copy_to_dist_dir(
        name = "{}_display_drivers_dist".format(kernel_build_tv),
        data = all_module_rules,
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
        log = "info",
    )

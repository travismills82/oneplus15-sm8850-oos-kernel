load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load(":geas/cpu/oplus_geas_cpu_local_modules.bzl", "define_oplus_geas_cpu_local_modules")
load(":geas/system/oplus_geas_system_local_modules.bzl", "define_oplus_geas_system_local_modules")

def define_oplus_geas_local_modules():
    define_oplus_geas_cpu_local_modules()
    define_oplus_geas_system_local_modules()
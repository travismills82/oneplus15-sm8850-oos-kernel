load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module", "oplus_ddk_get_target", "oplus_ddk_get_variant", "bazel_support_platform")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():
    target = oplus_ddk_get_target()
    variant  = oplus_ddk_get_variant()
    kernel_build_variant = "{}_{}".format(target, variant)

    if bazel_support_platform == "qcom" :
        zram_opt_ko_deps = ["//soc-repo:{}/drivers/block/zram/zram".format(kernel_build_variant),"//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",":oplus_bsp_mm_osvelte"]
        hybridswap_zram_ko_deps = []

#    define_oplus_ddk_module(
#        name = "oplus_bsp_memleak_detect_simple",
#        srcs = native.glob([
#            "**/*.h",
#            "memleak_detect/slub_track_simple.c",
#            "memleak_detect/vmalloc_track_simple.c",
#        ]),
#        includes = ["."],
#        )
#

    define_oplus_ddk_module(
        name = "oplus_bsp_sigkill_diagnosis",
        srcs = native.glob([
            "sigkill_diagnosis/sigkill_diagnosis.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_zram_opt",
        srcs = native.glob([
            "**/*.h",
            "zram_opt/zram_opt.c",
        ]),
        includes = ["."],
        ko_deps = zram_opt_ko_deps,
        local_defines = ["CONFIG_DYNAMIC_TUNING_SWAPPINESS", "CONFIG_OPLUS_BALANCE_ANON_FILE_RECLAIM", "CONFIG_HYBRIDSWAP_SWAPD", "CONFIG_OPLUS_EXTRA_FREE_KBYTES"],
#        copts = select({
#            "//build/kernel/kleaf:kocov_is_true": ["-fprofile-arcs", "-ftest-coverage"],
#            "//conditions:default": [],
#        }),
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_proactive_compact",
        srcs = native.glob([
            "**/*.h",
            "proactive_compact/proactive_compact.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_kshrink_slabd",
        srcs = native.glob([
            "**/*.h",
            "async_reclaim_opt/kshrink_slabd/kshrink_slabd.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_uxmem_opt",
        srcs = native.glob([
            "**/*.h",
            "uxmem_opt/uxmem_opt.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_UXMEM_OPT"],
        ko_deps = [":oplus_bsp_mm_osvelte", "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_mglru_opt",
        srcs = native.glob([
            "**/*.h",
            "mglru_opt/mglru_opt.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_MGLRU_OPT"],
        ko_deps = ["//vendor/oplus/kernel/mm:oplus_bsp_mm_osvelte"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_dynamic_readahead",
        srcs = native.glob([
            "**/*.h",
            "dynamic_readahead/dynamic_readahead.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_DYNAMIC_READAHEAD"],
        ko_deps = ["//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_pcppages_opt",
        srcs = native.glob([
            "**/*.h",
            "async_reclaim_opt/pcppages_opt/pcppages_opt.c",
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_kswapd_opt",
        srcs = native.glob([
            "kswapd_opt/kswapd_opt.c",
        ]),
        includes = ["."],
        local_defines = ["CONFIG_OPLUS_FEATURE_KSWAPD_OPT", "CONFIG_COSTLY_ALLOC_MASK_RECLAIM"],
        conditional_defines = {
            "qcom": ["CONFIG_QCOM_ALLOC_MASK_RECLAIM"],
        },
#       copts = select({
#           "//build/kernel/kleaf:kocov_is_true": ["-fprofile-arcs", "-ftest-coverage"],
#           "//conditions:default": [],
#       }),
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_zstdn",
        srcs = native.glob([
            "**/*.h",
            "hybridswap_zram/zstd/include/*.h",
            "hybridswap_zram/zstd/common/*.h",
            "hybridswap_zram/zstd/compress/*.h",
            "hybridswap_zram/zstd/decompress/*.h",
            "hybridswap_zram/zstd/crypto_zstd.c",
            "hybridswap_zram/zstd/zstd_compress_module.c",
            "hybridswap_zram/zstd/xxhash.c",
            "hybridswap_zram/zstd/common/debug.c",
            "hybridswap_zram/zstd/common/entropy_common.c",
            "hybridswap_zram/zstd/common/error_private.c",
            "hybridswap_zram/zstd/common/fse_decompress.c",
            "hybridswap_zram/zstd/common/zstd_common.c",
            "hybridswap_zram/zstd/compress/fse_compress.c",
            "hybridswap_zram/zstd/compress/hist.c",
            "hybridswap_zram/zstd/compress/huf_compress.c",
            "hybridswap_zram/zstd/compress/zstd_compress.c",
            "hybridswap_zram/zstd/compress/zstd_compress_literals.c",
            "hybridswap_zram/zstd/compress/zstd_compress_sequences.c",
            "hybridswap_zram/zstd/compress/zstd_compress_superblock.c",
            "hybridswap_zram/zstd/compress/zstd_double_fast.c",
            "hybridswap_zram/zstd/compress/zstd_fast.c",
            "hybridswap_zram/zstd/compress/zstd_lazy.c",
            "hybridswap_zram/zstd/compress/zstd_ldm.c",
            "hybridswap_zram/zstd/compress/zstd_opt.c",
            "hybridswap_zram/zstd/zstd_decompress_module.c",
            "hybridswap_zram/zstd/decompress/huf_decompress.c",
            "hybridswap_zram/zstd/decompress/zstd_ddict.c",
            "hybridswap_zram/zstd/decompress/zstd_decompress.c",
            "hybridswap_zram/zstd/decompress/zstd_decompress_block.c"
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_memleak_detect",
        srcs = native.glob([
            "**/*.h",
            "memleak_detect/slub_track.c",
            "memleak_detect/vmalloc_track.c",
            "memleak_detect/memleak_debug_stackdepot.c"
        ]),
        includes = ["."],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_mm_osvelte",
        srcs = native.glob([
            "mm_osvelte/common.c",
            "mm_osvelte/logger.c",
            "mm_osvelte/lowmem-dbg.c",
            "mm_osvelte/mm-config.c",
            "mm_osvelte/proc-memstat.c",
            "mm_osvelte/sys-ashmem.c",
            "mm_osvelte/sys-dmabuf.c",
            "mm_osvelte/sys-memstat.c",
            "mm_osvelte/vsprintf-dup.c",
            "mm_osvelte/common.h",
            "mm_osvelte/internal.h",
            "mm_osvelte/logger.h",
            "mm_osvelte/lowmem-dbg.h",
            "mm_osvelte/memstat.h",
            "mm_osvelte/mm-config.h",
            "mm_osvelte/mm-trace.h",
            "mm_osvelte/proc-memstat.h",
            "mm_osvelte/sys-memstat.h",
            "mm_osvelte/mm-hooks.h",
            "mm_osvelte/hooks.c",
        ]),
        includes = ["."],
        conditional_defines = {
            "qcom":  [ "CONFIG_OPLUS_VENDOR_QCOM" ],
            "mtk": [ "CONFIG_OPLUS_VENDOR_MTK" ],
        },
        local_defines = ["CONFIG_OPLUS_FEATURE_MM_BOOSTPOOL"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_zstdn_o",
        srcs = native.glob([
            "**/*.h",
            "zstd_o/include/*.h",
            "zstd_o/common/*.h",
            "zstd_o/compress/*.h",
            "zstd_o/decompress/*.h",
            "zstd_o/crypto_zstd.c",
            "zstd_o/zstd_compress_module.c",
            "zstd_o/xxhash.c",
            "zstd_o/common/debug.c",
            "zstd_o/common/entropy_common.c",
            "zstd_o/common/error_private.c",
            "zstd_o/common/fse_decompress.c",
            "zstd_o/common/zstd_common.c",
            "zstd_o/compress/fse_compress.c",
            "zstd_o/compress/hist.c",
            "zstd_o/compress/huf_compress.c",
            "zstd_o/compress/zstd_compress.c",
            "zstd_o/compress/zstd_compress_literals.c",
            "zstd_o/compress/zstd_compress_sequences.c",
            "zstd_o/compress/zstd_compress_superblock.c",
            "zstd_o/compress/zstd_double_fast.c",
            "zstd_o/compress/zstd_fast.c",
            "zstd_o/compress/zstd_lazy.c",
            "zstd_o/compress/zstd_ldm.c",
            "zstd_o/compress/zstd_opt.c",
            "zstd_o/zstd_decompress_module.c",
            "zstd_o/decompress/huf_decompress.c",
            "zstd_o/decompress/zstd_ddict.c",
            "zstd_o/decompress/zstd_decompress.c",
            "zstd_o/decompress/zstd_decompress_block.c"
        ]),
        includes = ["."],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_bsp_mm",
        module_list = [
#            "oplus_bsp_memleak_detect_simple",
            "oplus_bsp_sigkill_diagnosis",
            "oplus_bsp_zram_opt",
            "oplus_bsp_proactive_compact",
#            "oplus_bsp_hybridswap_zram",
            "oplus_bsp_uxmem_opt",
            "oplus_bsp_dynamic_readahead",
            "oplus_bsp_kswapd_opt",
            "oplus_bsp_pcppages_opt",
            "oplus_bsp_kshrink_slabd",
            "oplus_bsp_memleak_detect",
            "oplus_bsp_zstdn",
            "oplus_bsp_mm_osvelte",
            "oplus_bsp_zstdn_o",
            "oplus_bsp_mglru_opt",
        ],
    )

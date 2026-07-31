def register_modules(registry):
    registry.register(
        name = "drivers/block/zram/zram",
        out = "zram.ko",
        config = "CONFIG_ZRAM",
        srcs = [
            # do not sort
            "drivers/block/zram/zcomp.c",
            "drivers/block/zram/zram_drv.c",
            "drivers/block/zram/zcomp.h",
            "drivers/block/zram/zram_drv.h",
            "drivers/soc/qcom/qpace/qpace.h",
            "drivers/block/zram/hybridswap_zram_drv.h",
            "drivers/block/zram/hybridswap/hybridmain.c",
            "drivers/block/zram/hybridswap/hybridswapd.c",
            "drivers/block/zram/hybridswap/hybridswap.c",
            "drivers/block/zram/hybridswap/display.c",
            "drivers/block/zram/hybridswap/hybridswap.h",
            "drivers/block/zram/hybridswap/internal.h",
            "drivers/block/zram/hybridswap/header_dup.h",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/qpace/qpace_drv",
            "mm/zsmalloc",
            "drivers/soc/qcom/panel_event_notifier",
            "//vendor/oplus/kernel/mm:oplus_bsp_mm_osvelte",
            "drivers/dma-buf/heaps/qcom_dma_heaps",
            "//vendor/oplus/kernel/cpu:oplus_bsp_sched_assist",
        ],
        local_defines = [
            "CONFIG_HYBRIDSWAP",
            "CONFIG_HYBRIDSWAP_SWAPD",
            "CONFIG_HYBRIDSWAP_CORE",
            "CONFIG_OPLUS_QPACE_RUS",
            "CONFIG_OPLUS_FEATURE_MM_EZRECLAIMD",
            "CONFIG_QCOM_PANEL_EVENT_NOTIFIER",
        ],
    )

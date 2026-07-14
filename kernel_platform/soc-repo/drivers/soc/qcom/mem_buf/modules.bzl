def register_modules(registry):
    registry.register(
        name = "drivers/soc/qcom/mem_buf/mem_buf_dev",
        out = "mem_buf_dev.ko",
        config = "CONFIG_QCOM_MEM_BUF_DEV",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mem_buf/mem-buf-dev.c",
            "drivers/soc/qcom/mem_buf/mem-buf-dev.h",
            "drivers/soc/qcom/mem_buf/mem-buf-gh.h",
            "drivers/soc/qcom/mem_buf/mem-buf-ids.c",
            "drivers/soc/qcom/mem_buf/mem-buf-ids.h",
            "drivers/soc/qcom/mem_buf/mem_buf_dma_buf.c",
            "drivers/dma-buf/heaps/qcom_sg_ops.h",
            "drivers/dma-buf/heaps/qcom_dma_heap_priv.h",
            "drivers/dma-buf/heaps/deferred-free-helper.h",
        ],
        conditional_srcs = {
            "CONFIG_QCOM_MEM_BUF_DEV_GH": {
                True: [
                    # do not sort
                    "drivers/soc/qcom/mem_buf/mem-buf-dev-gh.c",
                    "drivers/soc/qcom/mem_buf/mem-buf-msgq.h",
                    "drivers/soc/qcom/mem_buf/trace-mem-buf.h",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_mem_notifier",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
            "drivers/virt/gunyah/gh_rm_heap_manager",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mem_buf/mem_buf",
        out = "mem_buf.ko",
        config = "CONFIG_QCOM_MEM_BUF",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mem_buf/mem-buf-dev.h",
            "drivers/soc/qcom/mem_buf/mem-buf-gh.h",
            "drivers/soc/qcom/mem_buf/mem-buf-ids.h",
            "drivers/soc/qcom/mem_buf/mem-buf.c",
            "drivers/dma-buf/heaps/deferred-free-helper.h",
        ],
        conditional_srcs = {
            "CONFIG_QCOM_MEM_BUF_GH": {
                True: [
                    # do not sort
                    "drivers/soc/qcom/mem_buf/mem-buf-gh.c",
                    "drivers/dma-buf/heaps/qcom_dma_heap_priv.h",
                    "drivers/dma-buf/heaps/qcom_sg_ops.h",
                    "drivers/soc/qcom/mem_buf/mem-buf-msgq.h",
                    "drivers/soc/qcom/mem_buf/trace-mem-buf.h",
                ],
            },
        },
        deps = [
            # do not sort
            "drivers/dma-buf/heaps/qcom_dma_heaps",
            "drivers/soc/qcom/mem_buf/mem_buf_dev",
            "drivers/soc/qcom/mem_buf/mem_buf_msgq",
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gunyah_loader",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

    registry.register(
        name = "drivers/soc/qcom/mem_buf/mem_buf_msgq",
        out = "mem_buf_msgq.ko",
        config = "CONFIG_QCOM_MEM_BUF_MSGQ",
        srcs = [
            # do not sort
            "drivers/soc/qcom/mem_buf/mem-buf-msgq.c",
            "drivers/soc/qcom/mem_buf/mem-buf-msgq.h",
            "drivers/soc/qcom/mem_buf/trace-mem-buf.h",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/mem_buf/mem_buf_dev",
            "drivers/soc/qcom/secure_buffer",
            "drivers/firmware/qcom/qcom-scm",
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "drivers/virt/gunyah/gh_dbl",
            "arch/arm64/gunyah/gh_arm_drv",
        ],
    )

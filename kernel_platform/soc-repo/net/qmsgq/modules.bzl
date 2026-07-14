def register_modules(registry):
    registry.register(
        name = "net/qmsgq/af_qmsgq",
        out = "af_qmsgq.ko",
        config = "CONFIG_QMSGQ",
        srcs = [
            # do not sort
            "net/qmsgq/af_qmsgq.h",
            "net/qmsgq/af_qmsgq.c",
            "net/qmsgq/qmsgq_transport.h",
            "net/qmsgq/qmsgq_gunyah.h",
            "net/qmsgq/qmsgq_transport.c",
        ],
        deps = [
            # do not sort
            "net/vmw_vsock/vsock",
        ],
    )

    registry.register(
        name = "net/qmsgq/qmsgq_gunyah",
        out = "qmsgq_gunyah.ko",
        config = "CONFIG_QMSGQ_GUNYAH",
        srcs = [
            # do not sort
            "net/qmsgq/af_qmsgq.h",
            "net/qmsgq/qmsgq_transport.h",
            "net/qmsgq/qmsgq_gunyah.h",
            "net/qmsgq/qmsgq_gunyah.c",
        ],
        deps = [
            # do not sort
            "drivers/virt/gunyah/gh_rm_drv",
            "drivers/virt/gunyah/gh_msgq",
            "net/qmsgq/af_qmsgq",
            "net/vmw_vsock/vsock",
        ],
    )

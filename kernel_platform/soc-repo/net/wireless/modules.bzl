def register_modules(registry):
    registry.register(
        name = "net/wireless/cfg80211",
        out = "cfg80211.ko",
        config = "CONFIG_CFG80211",
        srcs = [
            # do not sort
            "net/wireless/ap.c",
            "net/wireless/chan.c",
            "net/wireless/core.c",
            "net/wireless/core.h",
            "net/wireless/debugfs.h",
            "net/wireless/ethtool.c",
            "net/wireless/ibss.c",
            "net/wireless/mesh.c",
            "net/wireless/mlme.c",
            "net/wireless/nl80211.c",
            "net/wireless/nl80211.h",
            "net/wireless/ocb.c",
            "net/wireless/pmsr.c",
            "net/wireless/radiotap.c",
            "net/wireless/rdev-ops.h",
            "net/wireless/reg.c",
            "net/wireless/reg.h",
            "net/wireless/scan.c",
            "net/wireless/sme.c",
            "net/wireless/sysfs.c",
            "net/wireless/sysfs.h",
            "net/wireless/trace.c",
            "net/wireless/trace.h",
            "net/wireless/util.c",
            "net/wireless/wext-compat.h",
            "net/wireless/shipped-certs",
        ],
        conditional_srcs = {
            "CONFIG_OF": {
                True: [
                    # do not sort
                    "net/wireless/of.c",
                ],
            },
            "CONFIG_CFG80211_DEBUGFS": {
                True: [
                    # do not sort
                    "net/wireless/debugfs.c",
                ],
            },
            "CONFIG_CFG80211_WEXT": {
                True: [
                    # do not sort
                    "net/wireless/wext-compat.c",
                    "net/wireless/wext-sme.c",
                ],
            },
        },
        deps = [
            # do not sort
            "net/rfkill/rfkill",
        ],
    )

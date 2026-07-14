load(":net/mac80211/modules.bzl", register_mac80211 = "register_modules")
load(":net/qmsgq/modules.bzl", register_qmsgq = "register_modules")
load(":net/qrtr/modules.bzl", register_qrtr = "register_modules")
load(":net/wireless/modules.bzl", register_wireless = "register_modules")

def register_modules(registry):
    register_mac80211(registry)
    register_qmsgq(registry)
    register_qrtr(registry)
    register_wireless(registry)

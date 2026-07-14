def register_modules(registry):
    registry.register(
        name = "drivers/rtc/rtc-pm8xxx",
        out = "rtc-pm8xxx.ko",
        config = "CONFIG_RTC_DRV_PM8XXX",
        srcs = [
            # do not sort
            "drivers/rtc/rtc-pm8xxx.c",
        ],
    )

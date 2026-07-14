def register_modules(registry):
    registry.register(
        name = "sound/soc/codecs/snd-soc-hdmi-codec",
        out = "snd-soc-hdmi-codec.ko",
        config = "CONFIG_SND_SOC_HDMI_CODEC",
        srcs = [
            # do not sort
            "sound/soc/codecs/hdmi-codec.c",
        ],
    )

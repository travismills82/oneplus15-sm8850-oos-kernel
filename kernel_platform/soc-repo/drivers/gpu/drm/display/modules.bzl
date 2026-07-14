def register_modules(registry):
    registry.register(
        name = "drivers/gpu/drm/display/drm_display_helper",
        out = "drm_display_helper.ko",
        config = "CONFIG_DRM_DISPLAY_HELPER",
        srcs = [
            # do not sort
            "drivers/gpu/drm/display/drm_display_helper_mod.c",
            "drivers/gpu/drm/display/drm_dp_helper_internal.h",
        ],
        conditional_srcs = {
            "CONFIG_DRM_DISPLAY_DP_HELPER": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_dp_dual_mode_helper.c",
                    "drivers/gpu/drm/display/drm_dp_helper.c",
                    "drivers/gpu/drm/display/drm_dp_mst_topology.c",
                    "drivers/gpu/drm/display/drm_dsc_helper.c",
                    "drivers/gpu/drm/display/drm_dp_mst_topology_internal.h",
                ],
            },
            "CONFIG_DRM_DISPLAY_DP_TUNNEL": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_dp_tunnel.c",
                ],
            },
            "CONFIG_DRM_DISPLAY_HDCP_HELPER": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_hdcp_helper.c",
                ],
            },
            "CONFIG_DRM_DISPLAY_HDMI_HELPER": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_hdmi_helper.c",
                    "drivers/gpu/drm/display/drm_scdc_helper.c",
                ],
            },
            "CONFIG_DRM_DP_AUX_CHARDEV": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_dp_aux_dev.c",
                ],
            },
            "CONFIG_DRM_DP_CEC": {
                True: [
                    # do not sort
                    "drivers/gpu/drm/display/drm_dp_cec.c",
                ],
            },
        },
    )

    registry.register(
        name = "drivers/gpu/drm/display/drm_dp_aux_bus",
        out = "drm_dp_aux_bus.ko",
        config = "CONFIG_DRM_DP_AUX_BUS",
        srcs = [
            # do not sort
            "drivers/gpu/drm/display/drm_dp_aux_bus.c",
        ],
    )

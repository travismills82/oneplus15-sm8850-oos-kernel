#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only

set -euo pipefail

readonly SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

export RELEASE_TAG='oos16.0.10.500-ack-6.12.27'
export FIRMWARE='OxygenOS 16.0.10.500(EX01)'
export BUILD_DISPLAY_ID='CPH2747_16.0.10.500(EX01)'
export DEVICE='OnePlus 15 / CPH2747 / Canoe'
export BOOT_PARTITION_BYTES='100663296'
export STOCK_SYSTEM_DLKM_BYTES='14131200'
export STOCK_SYSTEM_DLKM_BLOCKS='3450'
export STOCK_SYSTEM_DLKM_SHA256='18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7'
export STOCK_SYSTEM_DLKM_EROFS_MAGIC='e2e1f5e0'

exec "$SCRIPT_DIR/build-oos16.0.9.400-twrp-installer.sh" "$@"

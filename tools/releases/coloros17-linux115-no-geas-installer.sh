# SPDX-License-Identifier: GPL-2.0-only
# Sourced only after the ZIP entrypoint checks this helper and its constants.
# No test backend, target override or verification-disable option is packaged.

canary_fail() {
    ui_print "ERROR: $*"
    if [ "${WRITE_STARTED:-0}" = 1 ]; then
        WRITE_STARTED=0
        trap '' HUP INT TERM
        if canary_restore; then
            ui_print 'The immediate pre-install boot/vendor pair was restored and verified.'
        else
            ui_print "ROLLBACK FAILED. DO NOT REBOOT. Preserve $BACKUP_DIR and use host recovery."
        fi
    fi
    exit 1
}

canary_hash() { sha256sum "$1" | awk '{print $1}'; }
canary_size() { wc -c < "$1" | tr -d '[:space:]'; }
canary_is_block() { [ -b "$1" ]; }
canary_property() { getprop "$1"; }
canary_mounts() { cat /proc/mounts; }
canary_bootconfig() { cat /proc/bootconfig; }
canary_root_uid() { id -u; }
canary_kernel() { uname -r; }
canary_battery_capacity() { cat /sys/class/power_supply/battery/capacity; }
canary_battery_temperature() { cat /sys/class/power_supply/battery/temp; }
canary_capacity() { blockdev --getsize64 "$1"; }
canary_vendor_table() { dmctl table vendor_dlkm_b; }
canary_super_device() { cat "/sys/class/block/$(basename "$SUPER")/dev"; }
canary_snapshot_state() { cat /metadata/ota/state; }
canary_snapshot_count() { find /metadata/ota/snapshots -mindepth 1 -maxdepth 1 -print | wc -l; }
canary_snapshot_present() { [ -f /metadata/ota/state ] && [ -d /metadata/ota/snapshots ]; }
canary_free_kb() { df -Pk "$1" | awk 'NR == 2 {print $4}'; }
canary_data_root() { printf '/data/media/0\n'; }
canary_data_filesystem() { df -Pk "$1" | awk 'NR == 2 {print $1}'; }
canary_resolve_path() { readlink -f "$1"; }
canary_vendor_alias_exists() { [ -b /dev/block/mapper/vendor_dlkm_a ]; }
canary_no_holders() {
    holders=$(ls -A "/sys/class/block/$(basename "$1")/holders") || return 1
    [ -z "$holders" ]
}

canary_partition() {
    # Logical names never go through by-name fallback or a caller-selected path.
    case "$1" in
        vendor_dlkm_b|system_dlkm_b|my_manifest_b) p="/dev/block/mapper/$1" ;;
        boot_a|boot_b|vbmeta_a|vbmeta_b|init_boot_a|init_boot_b|vendor_boot_b|dtbo_b|recovery_a|recovery_b|super)
            p="/dev/block/by-name/$1" ;;
        *) return 1 ;;
    esac
    canary_is_block "$p" || return 1
    readlink -f "$p"
}

canary_prefix_hash() {
    dd if="$1" bs=4096 count=$((SUPER_PREFIX_BYTES / 4096)) 2>/dev/null | sha256sum | awk '{print $1}'
}

canary_expect() {
    got=$(canary_hash "$1") || canary_fail "Cannot hash $1"
    [ "$got" = "$2" ] || canary_fail "Hash mismatch for $1; unsupported or changed contract."
}

canary_payload() {
    [ "$(canary_size "$1")" = "$2" ] || canary_fail "Wrong payload length: $1"
    canary_expect "$1" "$3"
}

canary_unmounted() {
    canary_no_holders "$VENDOR" || canary_fail 'vendor_dlkm has holders; unmount it first.'
    canary_vendor_alias_exists && canary_fail 'Another vendor_dlkm mapping is present; unsupported shared-vendor state.'
    # Never unmount a caller's filesystem or remove/create a DM mapping.
    canary_mounts > "$WORKDIR/mounts"
    while read -r src rest; do
        real=$(readlink -f "$src" 2>/dev/null || true)
        [ "$real" != "$VENDOR" ] || canary_fail 'vendor_dlkm is mounted; unmount it in recovery first.'
    done < "$WORKDIR/mounts"
}

canary_firmware_display() {
    manifest=$(canary_partition my_manifest_b) || return 1
    fs=$(blkid -s TYPE -o value "$manifest") || return 1
    MANIFEST_MOUNT="$WORKDIR/firmware"
    mkdir "$MANIFEST_MOUNT" || return 1
    case "$fs" in
        erofs) mount -t erofs -o ro,nosuid,nodev,noexec "$manifest" "$MANIFEST_MOUNT" || return 1 ;;
        ext4) mount -t ext4 -o ro,noload,nosuid,nodev,noexec "$manifest" "$MANIFEST_MOUNT" || return 1 ;;
        *) return 1 ;;
    esac
    FIRMWARE_DISPLAY=$(awk -F= '$1 == "ro.build.display.id" {n++; v=substr($0,index($0,"=")+1)} END {if(n!=1) exit 1; print v}' "$MANIFEST_MOUNT/build.prop") || return 1
    umount "$MANIFEST_MOUNT" || return 1
    MANIFEST_MOUNT=''
}

canary_guards() {
    [ "$(canary_root_uid)" = 0 ] || canary_fail 'Recovery root is required.'
    capacity=$(canary_battery_capacity) || canary_fail 'Battery capacity is unavailable.'
    temperature=$(canary_battery_temperature) || canary_fail 'Battery temperature is unavailable.'
    case "$capacity:$temperature" in *[!0-9:]*|:|:*|*:) canary_fail 'Invalid battery telemetry.' ;; esac
    [ "$capacity" -ge "$MIN_BATTERY" ] && [ "$capacity" -le 100 ] || canary_fail 'Battery charge is too low for flashing.'
    [ "$temperature" -le "$MAX_BATTERY_TEMP" ] || canary_fail 'Let the battery cool before flashing.'
    [ "$(canary_property ro.boot.slot_suffix)" = _b ] || canary_fail 'Only the tested active slot B is supported; do not switch slots to bypass this.'
    case "$(canary_property ro.boot.slot)" in ''|b) ;; *) canary_fail 'Conflicting slot properties.' ;; esac
    [ "$(canary_property ro.twrp.version)" = "$RECOVERY" ] || canary_fail 'Unsupported recovery build.'
    [ "$(canary_kernel)" = "$RECOVERY_KERNEL" ] || canary_fail 'Unsupported recovery kernel.'
    case "$(canary_property ro.product.device)" in infiniti|OP611FL1|OP60FFL1) ;; *) canary_fail 'Not the qualified OnePlus 15 device family.' ;; esac
    canary_bootconfig > "$WORKDIR/bootconfig"
    grep -q '^androidboot.vbmeta.device_state = "unlocked"$' "$WORKDIR/bootconfig" || canary_fail 'Raw unlocked boot state unavailable.'
    grep -q '^androidboot.verifiedbootstate = "orange"$' "$WORKDIR/bootconfig" || canary_fail 'Raw ORANGE boot state unavailable.'
    canary_mounts > "$WORKDIR/mounts"
    awk '$2=="/tmp" && $3=="tmpfs" {ok=1} END {exit !ok}' "$WORKDIR/mounts" || canary_fail '/tmp must be tmpfs.'
    awk '$2=="/metadata" && $1 ~ /^\/dev\/block\// && ($3=="f2fs" || $3=="ext4") {ok=1} END {exit !ok}' "$WORKDIR/mounts" || canary_fail 'Real metadata is not mounted; snapshot safety is unknown.'
    canary_snapshot_present || canary_fail 'Snapshot state is unavailable.'
    snapshot_state=$(canary_snapshot_state) || canary_fail 'Cannot read OTA snapshot state.'
    [ -z "$snapshot_state" ] || canary_fail 'Active/nonempty OTA snapshot state.'
    snapshot_count=$(canary_snapshot_count | tr -d '[:space:]') || canary_fail 'Cannot inspect snapshot records.'
    [ "$snapshot_count" = 0 ] || canary_fail 'Snapshot records exist.'
    [ "$(canary_capacity "$BOOT")" = "$BOOT_BYTES" ] || canary_fail 'Boot partition capacity differs.'
    [ "$(canary_capacity "$VENDOR")" = "$VENDOR_BYTES" ] || canary_fail 'Vendor allocation differs; resizing is not supported.'
    [ "$(canary_capacity "$PARENT")" = "$PARENT_BYTES" ] || canary_fail 'Parent partition length differs.'
    [ "$BOOT" != "$(canary_partition boot_a)" ] || canary_fail 'Boot A/B alias the same block device.'
    canary_vendor_table > "$WORKDIR/vendor-table"
    [ "$(wc -l < "$WORKDIR/vendor-table" | tr -d '[:space:]')" = 2 ] || canary_fail 'Unexpected vendor mapping target count.'
    expected="0-$VENDOR_SECTORS: linear, $(canary_super_device) $VENDOR_START"
    [ "$(sed -n '2p' "$WORKDIR/vendor-table")" = "$expected" ] || canary_fail 'Vendor mapping is not the tested linear super extent.'
    canary_unmounted
    parent_hash=$(canary_hash "$PARENT") || canary_fail 'Cannot read the existing parent.'
    [ "$parent_hash" = "$PARENT_SHA" ] || canary_fail 'Existing vbmeta does not match. This package cannot install over stock/unmatched parent metadata and will not change it.'
    canary_expect "$VENDOR" "$EXISTING_VENDOR_SHA"
    [ "$(canary_prefix_hash "$SUPER")" = "$SUPER_PREFIX_SHA" ] || canary_fail 'Super metadata/layout differs from the tested canary.'
    for entry in $COMPANIONS; do
        part=${entry%%:*}; want=${entry#*:}
        path=$(canary_partition "$part") || canary_fail "Missing companion $part"
        canary_expect "$path" "$want"
    done
}

canary_protected_hashes() {
    for part in boot_a vbmeta_a vbmeta_b init_boot_a init_boot_b vendor_boot_b dtbo_b recovery_a recovery_b system_dlkm_b; do
        path=$(canary_partition "$part") || return 1
        sha256sum "$path" || return 1
    done
}

canary_backup_ready() {
    [ "$(canary_property twrp.user.0.decrypt)" = 1 ] || canary_fail 'Decrypt user0 in TWRP before installing; do not format data.'
    canary_mounts > "$WORKDIR/mounts"
    awk '$2=="/data" && $3=="f2fs" && $1 ~ /^\/dev\/block\// {ok=1} END {exit !ok}' "$WORKDIR/mounts" || canary_fail 'Decrypted F2FS data mount is required for durable backups.'
    CANARY_DATA_ROOT=$(canary_data_root) || canary_fail 'Cannot resolve backup storage.'
    [ -d "$CANARY_DATA_ROOT" ] && [ -w "$CANARY_DATA_ROOT" ] || canary_fail 'Decrypted durable backup storage unavailable.'
    data_source=$(awk '$2=="/data" {print $1}' "$WORKDIR/mounts") || canary_fail 'Cannot resolve the data mount.'
    backup_source=$(canary_data_filesystem "$CANARY_DATA_ROOT") || canary_fail 'Cannot identify the backup filesystem.'
    data_real=$(canary_resolve_path "$data_source") || canary_fail 'Cannot resolve the data block device.'
    backup_real=$(canary_resolve_path "$backup_source") || canary_fail 'Cannot resolve the backup block device.'
    case "$backup_real" in /dev/block/*) ;; *) canary_fail 'Backup destination is not durable userdata.' ;; esac
    [ "$backup_real" = "$data_real" ] || canary_fail 'Backup destination is on a different/shadow filesystem.'
    free=$(canary_free_kb "$CANARY_DATA_ROOT") || canary_fail 'Cannot inspect backup capacity.'
    case "$free" in ''|*[!0-9]*) canary_fail 'Invalid backup free-space result.' ;; esac
    [ "$free" -ge "$(((BOOT_BYTES + VENDOR_BYTES) / 1024 + 16384))" ] || canary_fail 'Insufficient durable backup space.'
}

canary_backup() {
    canary_backup_ready
    base="$CANARY_DATA_ROOT/TWRP/kernel-flash-backups"
    mkdir -p "$base" || canary_fail 'Cannot create backup root.'
    BACKUP_DIR="$base/coloros17-linux115-no-geas-b-$(date +%Y%m%d-%H%M%S)-$$"
    mkdir "$BACKUP_DIR" || canary_fail 'Cannot create unique backup directory.'
    canary_protected_hashes > "$BACKUP_DIR/protected.sha256" || canary_fail 'Cannot record protected partitions.'
    for name in vendor_dlkm boot; do
        case "$name" in vendor_dlkm) path=$VENDOR; bytes=$VENDOR_BYTES ;; boot) path=$BOOT; bytes=$BOOT_BYTES ;; esac
        before=$(canary_hash "$path") || canary_fail 'Cannot hash backup source.'
        dd if="$path" of="$BACKUP_DIR/$name.img" bs=4096 count=$((bytes / 4096)) conv=fsync || canary_fail "Cannot back up $name"
        [ "$(canary_size "$BACKUP_DIR/$name.img")" = "$bytes" ] || canary_fail 'Incomplete durable backup.'
        canary_expect "$BACKUP_DIR/$name.img" "$before"
        canary_expect "$path" "$before"
        printf '%s  %s.img\n' "$before" "$name" >> "$BACKUP_DIR/SHA256SUMS" || canary_fail 'Cannot record backup digest.'
    done
    printf 'profile=%s\nslot=_b\nparent_read_only_sha256=%s\nsuper_prefix_sha256=%s\nresult=BACKUP_VERIFIED\n' "$PROFILE" "$PARENT_SHA" "$SUPER_PREFIX_SHA" > "$BACKUP_DIR/manifest.txt" || canary_fail 'Cannot record backup identity.'
    sync || canary_fail 'Cannot sync backups.'
    ui_print "Verified durable backups: $BACKUP_DIR"
}

canary_write() {
    # This whitelist is enforced at the actual write boundary, including rollback.
    case "$1" in vendor_dlkm) dest=$VENDOR; bytes=$VENDOR_BYTES ;; boot) dest=$BOOT; bytes=$BOOT_BYTES ;; *) return 1 ;; esac
    [ "$(canary_size "$2")" = "$bytes" ] || return 1
    [ "$(canary_hash "$2")" = "$3" ] || return 1
    dd if="$2" of="$dest" bs=1048576 conv=fsync || return 1
    sync || return 1
    [ "$(canary_hash "$dest")" = "$3" ] || return 1
    ui_print "Full readback verified: $1"
}

canary_restore() {
    ui_print 'Restoring only the verified immediate boot/vendor backups; parent remains untouched.'
    (cd "$BACKUP_DIR" && sha256sum -c SHA256SUMS) || return 1
    old_vendor=$(awk '$2=="vendor_dlkm.img" {print $1}' "$BACKUP_DIR/SHA256SUMS")
    old_boot=$(awk '$2=="boot.img" {print $1}' "$BACKUP_DIR/SHA256SUMS")
    canary_write vendor_dlkm "$BACKUP_DIR/vendor_dlkm.img" "$old_vendor" || return 1
    canary_write boot "$BACKUP_DIR/boot.img" "$old_boot" || return 1
    sha256sum -c "$BACKUP_DIR/protected.sha256" || return 1
    [ "$(canary_prefix_hash "$SUPER")" = "$SUPER_PREFIX_SHA" ] || return 1
}

canary_install() {
    MODE=$1; WORKDIR=$2; WRITE_STARTED=0; BACKUP_DIR=''
    case "$MODE" in flash|dry-run) ;; *) canary_fail 'Invalid installation mode.' ;; esac
    BOOT=$(canary_partition boot_b) || canary_fail 'Cannot resolve boot B.'
    VENDOR=$(canary_partition vendor_dlkm_b) || canary_fail 'Cannot resolve linear vendor B.'
    PARENT=$(canary_partition vbmeta_b) || canary_fail 'Cannot read existing parent.'
    SUPER=$(canary_partition super) || canary_fail 'Cannot read super metadata.'
    canary_payload "$WORKDIR/boot.img" "$BOOT_BYTES" "$BOOT_SHA"
    canary_payload "$WORKDIR/vendor_dlkm.img" "$VENDOR_BYTES" "$VENDOR_SHA"
    canary_guards
    canary_firmware_display || canary_fail 'Cannot read the installed firmware manifest read-only.'
    [ "$FIRMWARE_DISPLAY" = "$FIRMWARE" ] || canary_fail 'Installed firmware is not the exact tested ColorOS build.'
    canary_protected_hashes > "$WORKDIR/protected.sha256" || canary_fail 'Cannot record protected state.'
    ui_print 'Restricted Linux 6.12.115 no-GEAS canary. Existing matching parent verified; it will NOT be written.'
    if [ "$MODE" = dry-run ]; then
        canary_backup_ready
        ui_print 'DRY RUN PASS: no backup/image/partition writes or slot changes.'
        return 0
    fi
    canary_backup
    trap 'canary_fail "Installation interrupted"' HUP INT TERM
    # Recheck the immutable prerequisites after backup, immediately before writing.
    canary_guards
    sha256sum -c "$BACKUP_DIR/protected.sha256" || canary_fail 'Protected state changed before writing.'
    WRITE_STARTED=1
    canary_write vendor_dlkm "$WORKDIR/vendor_dlkm.img" "$VENDOR_SHA" || canary_fail 'Vendor write/readback failed.'
    canary_write boot "$WORKDIR/boot.img" "$BOOT_SHA" || canary_fail 'Boot write/readback failed.'
    sha256sum -c "$BACKUP_DIR/protected.sha256" || canary_fail 'Protected partition changed.'
    [ "$(canary_prefix_hash "$SUPER")" = "$SUPER_PREFIX_SHA" ] || canary_fail 'Super metadata changed.'
    printf 'result=INSTALL_READBACK_VERIFIED\n' >> "$BACKUP_DIR/manifest.txt" || canary_fail 'Cannot record installation result.'
    sync || canary_fail 'Cannot sync final state.'
    WRITE_STARTED=0
    trap - HUP INT TERM
    ui_print 'INSTALL PASS: vendor_dlkm B and boot B verified. vbmeta/init_boot/slot/metadata unchanged.'
    ui_print 'Reboot System manually without changing slots. Keep the backups and host recovery available.'
}

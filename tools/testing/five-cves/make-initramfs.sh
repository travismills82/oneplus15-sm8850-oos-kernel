#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if [[ $# != 1 || $1 != /* ]]; then
    echo "Usage: $0 /absolute/disposable-output-directory" >&2
    exit 2
fi
out=$(realpath -m -- "$1")
name=${INITRAMFS_NAME:-initramfs}
[[ $name =~ ^[a-zA-Z0-9._-]+$ ]] || { echo "Invalid initramfs name" >&2; exit 2; }
root="$out/$name-root"
if [[ -e "$root" ]]; then
    echo "Refusing to overwrite existing initramfs-root: $root" >&2
    exit 2
fi
mkdir -p "$root"/{bin,sbin,usr/bin,usr/lib,lib,lib64,proc,sys,dev,run,tmp,etc,var/run}
copy_file() {
    local f=$1 dest=${2:-$1}
    mkdir -p -- "$root$(dirname -- "$dest")"
    cp -L -- "$f" "$root$dest"
}
copy_binary() {
    local f=$1 dest=${2:-$1} lib
    copy_file "$f" "$dest"
    while IFS= read -r lib; do
        [[ -n $lib ]] && copy_file "$lib"
    done < <(ldd "$f" 2>/dev/null | sed -nE 's/.*=> (\/[^ ]+).*/\1/p; s/^[[:space:]]*(\/[^ ]+).*/\1/p')
}
copy_file /bin/busybox
for app in sh mount umount mkdir cat grep head tail dmesg poweroff reboot sleep \
    uname wc kill insmod rmmod sync mdev ps sysctl sed awk find printf ping; do
    ln -s busybox "$root/bin/$app"
done
copy_binary "$(command -v ip)" /sbin/ip
copy_binary "$(command -v tc)" /sbin/tc
copy_binary "$(command -v dmsetup)" /sbin/dmsetup
copy_binary "$(command -v losetup)" /sbin/losetup
copy_binary "$(command -v iptables-legacy)" /sbin/iptables-legacy
ln -s iptables-legacy "$root/sbin/iptables"
xtables_dir=$(dirname "$(find /usr/lib /lib -path '*/xtables/libxt_CT.so' -print -quit 2>/dev/null)")
test -f "$xtables_dir/libxt_CT.so"
for f in "$xtables_dir"/*.so; do copy_binary "$f"; done
python=$(readlink -f "$(command -v python3)")
copy_binary "$python" /usr/bin/python3
stdlib=$(python3 -c 'import sysconfig; print(sysconfig.get_path("stdlib"))')
mkdir -p "$root$(dirname "$stdlib")"
cp -a "$stdlib" "$root$stdlib"
# Python extension DSOs have dependencies absent from ldd(python), notably
# libffi for ctypes and libcrypto for hashlib. Preserve these guest inputs.
while IFS= read -r extension; do
    copy_binary "$extension"
done < <(python3 -c 'import _ctypes, _hashlib; print(_ctypes.__file__); print(_hashlib.__file__)')
copy_file "$here/guest-tcp.py" /guest-tcp.py
copy_file "$here/guest-dm-tc.py" /guest-dm-tc.py
copy_file "$here/fixtures/cve80668-guest-test.py" /guest-test.py
test -f "$out/pair-test/cve80668_pair.ko"
copy_file "$out/pair-test/cve80668_pair.ko" /cve80668_pair.ko
copy_file "$here/guest-init.sh" /init
chmod 0755 "$root/init"
test -f "$out/workqueue-test/five_cves_workqueue.ko"
copy_file "$out/workqueue-test/five_cves_workqueue.ko" /five_cves_workqueue.ko
(cd "$root" && find . -print0 | cpio --null --quiet -o --format=newc) \
    | gzip -n > "$out/$name.cpio.gz"
sha256sum "$out/$name.cpio.gz"

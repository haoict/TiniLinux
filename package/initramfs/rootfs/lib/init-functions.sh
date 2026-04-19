#!/bin/busybox sh
log() {
    [ "$quiet" = "yes" ] || echo "$@"
}

mount_squashfs() {
    # wait for $bootpart and $overlayfs to be available
    i=1
    while [ $i -le 10 ]; do
        if [ -b "$bootpart" ] && [ "$overlayfs" = "tmpfs" ]; then
            break
        fi
        if [ -b "$bootpart" ] && [ -b "$overlayfs" ]; then
            break
        fi
        log "[initramfs] Waiting for boot partition $bootpart and overlay partition $overlayfs... ($i/10)"
        sleep 1
        i=$((i + 1))
    done

    log "[initramfs] Setting up OverlayFS: lower from $bootpart/$root, upper on $overlayfs"
    mkdir -p /mnt/boot /mnt/squashfs /mnt/overlayfs
    log "[initramfs] Mounting boot partition $bootpart..."
    mount -o rw "$bootpart" /mnt/boot || exec sh
    log "[initramfs] Mounting overlay partition $overlayfs (read-write)..."
    if [ "$overlayfs" = "tmpfs" ]; then
        mount -o rw -t tmpfs tmpfs /mnt/overlayfs || exec sh
    else
        mount -o rw "$overlayfs" /mnt/overlayfs || exec sh
    fi
    # Check for update
    if [ -f /mnt/overlayfs/.update/$root ]; then
        log "[initramfs] Found .update, applying update to /mnt/boot/$root..."
        rm -f /mnt/boot/$root
        mv /mnt/overlayfs/.update/$root /mnt/boot/$root
    fi
    log "[initramfs] Mounting root from squashfs image /mnt/boot/$root (read-only)..."
    mount -t squashfs -o loop,ro "/mnt/boot/$root" /mnt/squashfs || exec sh

    # Check for rootfs resize flags
    if [ -f /mnt/overlayfs/overlay_upper/root/.resize-rootfs ]; then
        mv /mnt/overlayfs/overlay_upper/root/.resize-rootfs /mnt/overlayfs/overlay_upper/root/.resize-rootfs-done
        if [ -f /mnt/overlayfs/overlay_upper/root/.resize-romsfs ]; then
            log "[initramfs] Found .resize-rootfs and .resize-romsfs flags - will boot into rootfs_romsfs resize mode"
            INIT_UNIT="--unit=fs-resize@rootfs_romsfs.service"
        else
            log "[initramfs] Found .resize-rootfs flag - will boot into rootfs resize mode"
            INIT_UNIT="--unit=fs-resize@rootfs.service"
        fi
        log "[initramfs] Unmounting overlay partition for resize..."
        umount $overlayfs || exec sh
        log "[initramfs] Moving squashfs mount to /newroot..."
        mount --move /mnt/squashfs /newroot || exec sh
    else
        mkdir -p /mnt/overlayfs/overlay_upper /mnt/overlayfs/overlay_workdir
        log "[initramfs] Mounting overlay filesystem to /newroot..."
        mount -t overlay overlay -o lowerdir=/mnt/squashfs,upperdir=/mnt/overlayfs/overlay_upper,workdir=/mnt/overlayfs/overlay_workdir /newroot || exec sh

        mkdir -p /newroot/mnt/squashfs /newroot/mnt/overlayfs
        log "[initramfs] Moving squashfs, overlay mount to /newroot/mnt/..."
        mount --move /mnt/squashfs /newroot/mnt/squashfs || exec sh
        mount --move /mnt/overlayfs /newroot/mnt/overlayfs || exec sh

        # Check for romsfs resize flag
        if [ -f /newroot/root/.resize-romsfs ]; then
            log "[initramfs] Found .resize-romsfs flag - will boot into romsfs resize mode"
            # /root/.resize-romsfs flag will be removed by fs-resize.sh after resizing
            INIT_UNIT="--unit=fs-resize@romsfs.service"
        else
            log "[initramfs] No resize flags found - proceeding with normal boot"
            INIT_UNIT="" # normal boot
        fi
    fi
}

mount_nfs() {
    # ---- NFS root boot ----
    # make sure nfs.ko, sunrpc.ko, and nfsv3.ko are either built into your kernel (=y) or present in the initramfs and loaded before the mount attempt. If they're modules you'll need modprobe nfs before the mount -t nfs call.
    # CONFIG_IP_PNP=y
    # CONFIG_NFS_FS=y
    # CONFIG_NFS_V3=y
    # CONFIG_ROOT_NFS=y
    # CONFIG_SUNRPC=y
    # host NFS rootfs prepration: sudo tar -p -xf images/rootfs.tar -C /srv/nfs/rootfs
    # host NFS must have insecure option: /etc/exports:
    # /srv/nfs/rootfs    10.0.2.0/24(rw,sync,no_subtree_check,no_root_squash,insecure)
    log "[initramfs] NFS root boot: $nfsroot"

    # wait for network interface
    i=1
    while [ $i -le 10 ]; do
        iface=$(ip link show | awk -F: '$2 ~ /^ e/{print $2; exit}' | tr -d ' ')
        [ -n "$iface" ] && break
        sleep 1; i=$((i+1))
    done
    # Bring up network
    # networkconf= format: networkconf=<ipaddr/mask>:<gw>
    # e.g. networkconf=dhcp  or  networkconf=192.168.1.10/24:192.168.1.1
    if [ "$networkconf" = "dhcp" ]; then
        log "[initramfs] Bringing up network via using interface: $iface"
        udhcpc -i "$iface" -t 5 -n -q || { log "[initramfs] DHCP failed"; exec sh; }
    else
        ipaddr=$(echo $networkconf | cut -d: -f1)
        gw=$(echo $networkconf     | cut -d: -f2)
        if [ -n "$ipaddr" ] && [ -n "$gw" ] && [ -n "$iface" ]; then
            log "[initramfs] Configuring static IP $ipaddr on $iface..."
            ip link set "$iface" up
            ip addr add "$ipaddr" dev "$iface"
            ip route add default via "$gw"
            # wait for carrier
            i=1
            while [ $i -le 5 ]; do
                [ "$(cat /sys/class/net/$iface/carrier 2>/dev/null)" = "1" ] && break
                sleep 1; i=$((i+1))
            done
            if [ "$(cat /sys/class/net/$iface/carrier 2>/dev/null)" != "1" ]; then
                log "[initramfs] No carrier on $iface (cable unplugged?)"
                exec sh
            fi
        else
            log "[initramfs] IP configuring failed, ipaddr=$ipaddr,gw=$gw,iface=$iface"; exec sh;
        fi
    fi

    log "[initramfs] Mounting NFS $nfs_server:$nfs_path..."
    i=1
    while [ $i -le 5 ]; do
        mount -t nfs -o "rw,noatime,nodiratime,vers=3,nolock,rsize=32768,wsize=32768,timeo=11,retrans=3" "$nfs_server:$nfs_path" /newroot && break
        log "[initramfs] NFS mount failed, retrying... ($i/5)"
        sleep 2; i=$((i+1))
    done

    if ! mountpoint -q /newroot; then
        log "[initramfs] NFS mount failed, dropping to shell"
        exec sh
    fi
}

mount_local_root() {
    # ---- local root ----
    # wait for $root to be available
    i=1
    while [ $i -le 30 ]; do
        if [ -b "$root" ]; then
            break
        fi
        log "[initramfs] Waiting for root device $root... ($i/30)"
        sleep 1
        i=$((i + 1))
    done

    if [ -n "$fsck_opt" ]; then
        log "[initramfs] (Not implemented) Running fsck (repair=$fsck_opt)..."
        ### We likely need to include the util-linux-fsck (e2fsck) package into the initramfs image. Busybox's fsck won't work
        # fsck -t ext4 $fsck_opt $root
        # ret=$?
        # log "[initramfs] fsck returned $ret"
        # if [ $ret -eq 2 ]; then
        #     log "[initramfs] fsck requested reboot..."
        #     reboot -f
        # elif [ $ret -gt 2 ]; then
        #     log "[initramfs] fsck failed (ret=$ret), dropping to shell"
        #     exec sh
        # fi
    fi

    log "[initramfs] No OverlayFS specified, using root=$root directly (r/w)."
    mount -o rw "$root" /newroot || exec sh

    # Check for rootfs resize flags
    if [ -f /newroot/root/.resize-rootfs ]; then
        log "[initramfs] Found .resize-rootfs flag - will boot into rootfs resize mode"
        mv /newroot/root/.resize-rootfs /newroot/root/.resize-rootfs-done
        if [ -f /newroot/root/.resize-romsfs ]; then
            INIT_UNIT="--unit=fs-resize@rootfs_romsfs.service"
        else
            INIT_UNIT="--unit=fs-resize@rootfs.service"
        fi
    elif [ -f /newroot/root/.resize-romsfs ]; then
        log "[initramfs] Found .resize-romsfs flag - will boot into romsfs resize mode"
        INIT_UNIT="--unit=fs-resize@romsfs.service"
    else
        log "[initramfs] No resize flags found - proceeding with normal boot"
        INIT_UNIT=""
    fi
}

do_switch_root() {
    log "[initramfs] Moving runtime directories mount to /newroot..."
    mount --move /proc /newroot/proc
    mount --move /sys /newroot/sys
    mount --move /dev /newroot/dev
    mount --move /run /newroot/run

    log "[initramfs] Init: ${init:-/sbin/init} $INIT_UNIT. Switching to real root..."
    exec switch_root /newroot ${init:-/sbin/init} $INIT_UNIT || exec sh
}

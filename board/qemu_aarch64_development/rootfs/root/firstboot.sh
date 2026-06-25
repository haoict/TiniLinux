#!/bin/sh

#systemctl enable --now dockerd.service

# fix nmconnection files permissions so it can autoconnect
chmod 600 /etc/NetworkManager/system-connections/*.nmconnection

systemctl disable --now nfs-blkmap
systemctl disable --now NetworkManager-initrd.service
systemctl daemon-reload

# Fix DNS
resolvectl dns eth0 8.8.8.8 8.8.4.4

# Done
mv /root/firstboot.sh /root/.firstboot-done.sh

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
# echo "Rebooting to complete first boot setup..." >/dev/tty0
# sleep 5
# systemctl reboot

#!/bin/sh

chown -R admin:tinilinux ${TARGET_DIR}/home/admin
chown -R viewer:tinilinux ${TARGET_DIR}/home/viewer

sleep 3
systemctl disable --now bluetooth

# Done
mv /root/firstboot.sh /root/.firstboot-done.sh

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
echo "Rebooting to complete first boot setup..." >/dev/tty0
sleep 5
systemctl enable dockerd.service
systemctl reboot

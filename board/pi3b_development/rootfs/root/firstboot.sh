#!/bin/sh

systemctl disable bluetooth
systemctl enable dockerd.service

# Done
mv /root/firstboot.sh /root/.firstboot-done.sh

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
echo "Rebooting to complete first boot setup..." >/dev/tty0
sleep 5
systemctl reboot

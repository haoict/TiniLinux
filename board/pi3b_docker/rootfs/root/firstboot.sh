#!/bin/sh
mv /root/firstboot.sh /root/.firstboot-done.sh

sleep 3
systemctl disable --now bluetooth

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
echo "Rebooting to complete first boot setup..."
sleep 10
systemctl reboot
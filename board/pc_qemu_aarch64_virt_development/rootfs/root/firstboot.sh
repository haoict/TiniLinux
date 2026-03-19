#!/bin/sh
touch /root/.vimrc
mv /root/firstboot.sh /root/.firstboot-done.sh

resolvectl dns eth0 8.8.8.8 8.8.4.4

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
echo "Rebooting to complete first boot setup..." >/dev/tty0
sleep 10
systemctl reboot
#!/bin/sh

# Fix vim
touch /root/.vimrc

# Fix users
chmod u+s /usr/bin/sudo
chmod u+s /usr/bin/passwd
chown -R admin:tinilinux /home/admin
chown -R viewer:tinilinux /home/viewer

# Fix DNS
resolvectl dns eth0 8.8.8.8 8.8.4.4

# Done
mv /root/firstboot.sh /root/.firstboot-done.sh

# FIXME: Workaround for docker/podman with /var directory as it always fails on firstboot. We need to reboot once to make it work.
echo "Rebooting to complete first boot setup..." >/dev/tty0
sleep 10
systemctl enable dockerd.service
systemctl reboot

#!/bin/sh

# fix nmconnection files permissions so it can autoconnect
chmod 600 /etc/NetworkManager/system-connections/*.nmconnection
# Done
mv /root/firstboot.sh /root/.firstboot-done.sh

systemctl disable --now NetworkManager-initrd.service
systemctl disable --now nfs-blkmap
# Persist /var
systemctl mask var.mount

# fsck /boot
fsck.fat -a $(cat /proc/cmdline | sed -n 's/.*bootpart=\([^ ]*\).*/\1/p')

echo "Rebooting..."
sleep 1
reboot

#!/bin/sh

# disable bluetooth
rfkill block bluetooth
systemctl disable --now bluetooth

# fix nmconnection files permissions so it can autoconnect
chmod 600 /etc/NetworkManager/system-connections/*.nmconnection
# Done
mv /root/firstboot.sh /root/.firstboot-done.sh
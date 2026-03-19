#!/bin/sh

# Fix vim
touch /root/.vimrc

# Fix users
chmod u+s /usr/bin/sudo
chmod u+s /usr/bin/passwd
chown -R admin:tinilinux /home/admin
chown -R viewer:tinilinux /home/viewer

# Done
mv /root/firstboot.sh /root/.firstboot-done.sh
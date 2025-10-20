#!/bin/bash

##################################################################
# Created by Christian Haitian for use to display a python       #
# based keyboard to the user.                                    #
##################################################################

# Offline install urwid==2.1.2
# With internet connection, we can run  pip install urwid==2.1.2
# But for offline env, we need to install it from .whl file
# Steps to create urwid-2.1.2-py3-none-any.whl (from another handheld machine with internet access):
# 1. Run: pip download urwid==2.1.2 --dest /root/urwid_offline
# 2. Run: cd /root/urwid_offline && tar xzf urwid-2.1.2.tar.gz && cd urwid-2.1.2
# 3. Run: python3 setup.py bdist_wheel
# 4. The .whl file will be created in the dist/ folder: dist/urwid-2.1.2-py3-none-any.whl
# 5. Copy the .whl file to the target system and install it using pip:
#    pip install urwid-2.1.2-py3-none-any.whl


chvt 6
sudo chmod 666 /dev/tty6
printf "\033c" >/dev/tty6

# Check /root/urwid-2.1.2-py3-none-any.whl exists, then install it
if [ -f /root/urwid-2.1.2-py3-none-any.whl  ]; then
  echo "Installing the python3 urwid module needed for this.  Please wait..." 2>&1 >/dev/tty6
  pip install /root/urwid-2.1.2-py3-none-any.whl 2>&1 >/dev/tty6
  # if installation successful, remove the .whl file to save space
  if [ $? -eq 0 ]; then
    mv /root/urwid-2.1.2-py3-none-any.whl /root/urwid-2.1.2-py3-none-any.whl.installed
  else
    echo "urwid module installation failed" 2>&1 >/dev/tty6
  fi
fi


export TERM=linux
export XDG_RUNTIME_DIR=/run/user/$UID/
export SDL_GAMECONTROLLERCONFIG_FILE="/root/gamecontrollerdb.txt"
sudo chmod 666 /dev/uinput
ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9
/usr/local/bin/gptokeyb2 "python3" -c "/root/gptokeyb2.ini" >/dev/null 2>&1 &

RESULTS="$(python3 /usr/local/bin/osk.py "$1" 2>&1 >/dev/tty6 | tail -n 1)"
ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9

echo "$RESULTS"
printf "\033c" > /dev/tty6

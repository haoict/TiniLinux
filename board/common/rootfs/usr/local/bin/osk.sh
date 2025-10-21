#!/bin/bash

##################################################################
# Created by Christian Haitian for use to display a python       #
# based keyboard to the user.                                    #
##################################################################

if [[ ! -e "/dev/input/by-path/platform-odroidgo2-joypad-event-joystick" ]]; then
  sudo setfont /usr/share/consolefonts/Lat7-TerminusBold28x14.psf.gz
else
  sudo setfont /usr/share/consolefonts/Lat7-Terminus16.psf.gz
fi

dpkg -s "python3-urwid" &>/dev/null
if [ "$?" != "0" ]; then
  echo "Installing the python3 urwid module needed for this.  Please wait..." 2>&1 >/dev/tty1
  sudo dpkg -i --force-all /opt/inttools/python3-urwid_2.0.1-2build2_arm64.deb
fi

sudo chmod 666 /dev/tty1
printf "\033c" > /dev/tty1

if [[ -z $(pgrep -f gptokeyb) ]] && [[ -z $(pgrep -f oga_controls) ]]; then
  sudo chmod 666 /dev/uinput
  export SDL_GAMECONTROLLERCONFIG_FILE="/opt/inttools/gamecontrollerdb.txt"
  /opt/inttools/gptokeyb -1 "python3" -c "/opt/inttools/keys.gptk" > /dev/null &
  disown
  set_gptokeyb="Y"
fi

export TERM=linux
export XDG_RUNTIME_DIR=/run/user/$UID/

if [[ ! -z $(pgrep -f osk.py) ]]; then
  pgrep -f osk.py | sudo xargs kill -9
  printf "\033c" > /dev/tty1
fi

RESULTS="$(/opt/inttools/osk.py "$1" 2>&1 >/dev/tty1 | tail -n 1)"
#sleep 1
if [[ ! -z "$set_gptokeyb" ]]; then
  pgrep -f gptokeyb | sudo xargs kill -9
  unset SDL_GAMECONTROLLERCONFIG_FILE
fi

if [[ ! -e "/dev/input/by-path/platform-odroidgo2-joypad-event-joystick" ]]; then
  sudo setfont /usr/share/consolefonts/Lat7-Terminus20x10.psf.gz
fi

echo "$RESULTS"
printf "\033c" > /dev/tty1
